/* engine/emit/routine.c — the explicit zero of a modifier's field in a brace
 * initializer (emit_internal.h; codegen design E5: one file per construction;
 * backend §5.10, rule 2).
 *
 * The state of a `routine.slot` is a `corot`, and zeroed it is ONGOING, the
 * right initial state without writing. The .k leaves it out, and the generated
 * C writes it:
 *
 *   array routine.slot Ag steps[3] = {       keel_routine_slot_ag2_Ag steps[3] = {
 *       { .f = hello, .ctx = g },                { .f = ag2_hello, .ctx = g, .state = {0} },
 *   };                                       };
 *
 * The field is read from the modifier's declaration in the generic module, and
 * the rule is the shape of that declaration, not the name `state`: a field the
 * element leaves out, held by value (no `*`), whose type is a single name the
 * generic module does not declare itself — a type of another module, like
 * `corot` in `pub modifier slot { routine f; C *ctx; corot state; }`. The
 * module's own `routine` and the binder `C *ctx` stay out, the first because
 * the module declares it, the second because it is a pointer.
 *
 * An element is a brace pair whose first token is a `.`: both the elements of
 * an array of the modifier and a single variable of it come out this way. A
 * declaration with no brace initializer, and `= {0}`, have no element and get
 * nothing. The insert goes on the line of the element's last value, so the line
 * mapping (§6) does not move. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/symtab.h"

#define NO_TOKEN ((size_t)-1)
#define MAX_FIELDS 16

/* one field of the modifier: its name token in the generic's tree */
typedef struct { KToken name; bool wanted; } Field;

static bool tok_is(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

static bool ident(const KAst *ast, size_t i) {
    return i < ast->tokens.len && k_token_is_ident(k_em_tok(ast, i));
}

/* a name or a dimension written as a number: what an instance's arguments are */
static bool word_or_number(const KAst *ast, size_t i) {
    if (i >= ast->tokens.len) return false;
    KToken t = k_em_tok(ast, i);
    if (t.len == 0) return false;
    char c = t.ptr[0];
    return k_token_is_ident(t) || (c >= '0' && c <= '9');
}

/* the token that closes the pair token `open` starts, or NO_TOKEN */
static size_t closer(const KAst *ast, size_t open, size_t limit) {
    int depth = 0;
    for (size_t i = open; i < limit; i++) {
        if (tok_is(ast, i, "{") || tok_is(ast, i, "(") || tok_is(ast, i, "[")) depth++;
        else if (tok_is(ast, i, "}") || tok_is(ast, i, ")") || tok_is(ast, i, "]")) {
            if (--depth == 0) return i;
        }
    }
    return NO_TOKEN;
}

/* whether the tree declares `name` at file level (the module's own name) */
static bool declared_in(const KAst *m, KToken name) {
    for (size_t k = 0; k < m->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&m->nodes, k);
        if (n->name_first >= n->name_end) continue;
        KToken t = k_em_tok(m, n->name_first);
        if (t.len == name.len && memcmp(t.ptr, name.ptr, name.len) == 0) return true;
    }
    return false;
}

/* The generic module an import alias names: the tree behind the module symbol
 * `alias`, as callee_module (uses.c) finds it. */
static const KAst *module_of(const KAst *ast, KToken alias) {
    if (!ast->symbols) return NULL;
    for (size_t i = 0; i < ast->symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(ast->symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin && s->origin->ast &&
            k_symtab_same_name(s->name, (keel_slice_char){ alias.len, (char *)alias.ptr }))
            return s->origin->ast;
    }
    return NULL;
}

/* The fields of the modifier `name` of the module `m` that an initializer has to
 * write (the shape above), in declaration order. Returns their count, or 0. */
static size_t fields_of(const KAst *m, KToken name, Field *out, size_t cap) {
    const KAstNode *mod = NULL;
    for (size_t k = 0; k < m->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&m->nodes, k);
        if (n->kind != K_AST_MODIFIER || n->name_first >= n->name_end) continue;
        KToken t = k_em_tok(m, n->name_first);
        if (t.len == name.len && memcmp(t.ptr, name.ptr, name.len) == 0) { mod = n; break; }
    }
    if (!mod || mod->body_first >= mod->body_end) return 0;
    size_t n = 0;
    /* the body is `{ <type> <decl>; ... }`: each field ends at a `;` or a `,` */
    size_t first = mod->body_first + 1, last = mod->body_end - 1;
    for (size_t i = first; i < last; ) {
        size_t end = i;
        while (end < last && !tok_is(m, end, ";") && !tok_is(m, end, ",")) end++;
        if (end == i) { i++; continue; }
        /* by value: no `*` anywhere in the field; the type is the single token
         * before the name, and the module must not declare it */
        bool value = true;
        for (size_t j = i; j < end; j++) if (tok_is(m, j, "*")) value = false;
        if (value && end - i == 2 && ident(m, i) && ident(m, end - 1) && !declared_in(m, k_em_tok(m, i))) {
            if (n >= cap) return 0;
            out[n++] = (Field){ .name = k_em_tok(m, end - 1), .wanted = true };
        }
        i = end + 1;
    }
    return n;
}

/* The modifier instance a type island heads: `<alias> . <modifier>`, with the
 * generic module behind the alias. False when the island is another shape. */
static bool instance_at(const KAst *ast, const KIsland *is, const KAst **m, KToken *modifier) {
    if (is->kind != K_ISLAND_TYPE || !ident(ast, is->anchor)) return false;
    if (!tok_is(ast, is->anchor + 1, ".") || !ident(ast, is->anchor + 2)) return false;
    const KAst *g = module_of(ast, k_em_tok(ast, is->anchor));
    if (!g) return false;
    *m = g;
    *modifier = k_em_tok(ast, is->anchor + 2);
    return true;
}

/* The brace initializer of the declaration the island heads: the `{` after the
 * `=` that follows it, or NO_TOKEN. What may stand between the type and the `=`
 * is only a declarator — names, `*` and dimensions —, so that a type island in
 * another position (a `foreach` binder) stops here. */
static size_t initializer(const KAst *ast, const KAstNode *n, size_t anchor) {
    for (size_t i = anchor + 3; i < n->end; i++) {
        if (tok_is(ast, i, "=")) return tok_is(ast, i + 1, "{") ? i + 1 : NO_TOKEN;
        if (tok_is(ast, i, "[")) {
            size_t c = closer(ast, i, n->end);
            if (c == NO_TOKEN) return NO_TOKEN;
            i = c;
            continue;
        }
        if (!word_or_number(ast, i) && !tok_is(ast, i, "*")) return NO_TOKEN;
    }
    return NO_TOKEN;
}

/* The fields the element [open, close] writes are struck off `f`; what is left
 * is appended before the `}`. */
static void element_edits(Emit *e, size_t open, size_t close, const Field *f, size_t nf,
                          size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    bool written[MAX_FIELDS] = { false };
    int depth = 0;
    for (size_t i = open; i <= close; i++) {
        if (tok_is(ast, i, "{") || tok_is(ast, i, "(") || tok_is(ast, i, "[")) depth++;
        else if (tok_is(ast, i, "}") || tok_is(ast, i, ")") || tok_is(ast, i, "]")) depth--;
        else if (depth == 1 && tok_is(ast, i, ".") && ident(ast, i + 1)) {
            KToken t = k_em_tok(ast, i + 1);
            for (size_t k = 0; k < nf; k++)
                if (f[k].name.len == t.len && memcmp(f[k].name.ptr, t.ptr, t.len) == 0) written[k] = true;
        }
    }
    size_t missing = 0;
    for (size_t k = 0; k < nf; k++) if (!written[k]) missing++;
    if (!missing) return;
    size_t last = close - 1;                /* the element's last value token */
    if (last <= open) return;               /* `{}`: nothing written, nothing to follow */
    size_t at = k_em_tok_end(ast, last);
    if (at <= a || at > b) return;
    /* `, .state = {0}` after the last value; after a trailing comma, no comma */
    bool comma = !tok_is(ast, last, ",");
    size_t need = 0;
    for (size_t k = 0; k < nf; k++) if (!written[k]) need += f[k].name.len + 11;
    char *w = k_em_scratch(e, need + 1);
    if (!w) return;
    size_t q = 0;
    for (size_t k = 0; k < nf; k++) {
        if (written[k]) continue;
        if (comma) { w[q++] = ','; }
        comma = true;
        w[q++] = ' ';
        w[q++] = '.';
        memcpy(w + q, f[k].name.ptr, f[k].name.len); q += f[k].name.len;
        memcpy(w + q, " = {0}", 6); q += 6;
    }
    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0, .text = w, .text_len = q });
}

void k_em_edits_routine(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->anchor < n->first || is->anchor >= n->end) continue;
        const KAst *m;
        KToken modifier;
        if (!instance_at(ast, is, &m, &modifier)) continue;
        Field f[MAX_FIELDS];
        size_t nf = fields_of(m, modifier, f, MAX_FIELDS);
        if (!nf) continue;
        size_t open = initializer(ast, n, is->anchor);
        if (open == NO_TOKEN) continue;
        size_t end = closer(ast, open, n->end);
        if (end == NO_TOKEN) continue;
        /* every element: a brace pair whose first token is a `.` */
        for (size_t i = open; i <= end; i++) {
            if (!tok_is(ast, i, "{") || !tok_is(ast, i + 1, ".")) continue;
            size_t c = closer(ast, i, end + 1);
            if (c == NO_TOKEN) break;
            element_edits(e, i, c, f, nf, a, b, ed, cnt);
            i = c;
        }
    }
}
