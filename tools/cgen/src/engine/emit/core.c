/* engine/emit/core.c — the verbs of the core (emit_internal.h; codegen design
 * E5: one file per construction; backend §5.3, rule 7; spec §4.2).
 *
 * `keel.length` is not a call to a module: it is the core's, and over an
 * `array` it is the table's own extent, so it comes out as a constant
 * expression and never reaches a function:
 *
 *   pub array i32 v[6] = { … };
 *   keel.length(v)              ->   sizeof(app_ofr_v)/sizeof(i32)
 *
 * The element type comes from the declaration, not from `*(v)` (rule 7), so a
 * column of pointers keeps its own type. The argument's bytes do not move: its
 * rename is the island the parser left on it.
 *
 * Refused (k_em_handles_core answers false, and the module is refused with
 * implementation-limit): every other verb of the core (`keel.capacity`,
 * `keel.dim`), `keel.length` over anything but a module `array` whose
 * dimensions are literal — a parameter with a dimension binder takes `keel__n`
 * (§5.18, rule 5) and has no golden —, and an element type that names a
 * declaration of the module, which would have to be renamed inside the
 * generated text. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define ARROW "\xE2\x86\x92"
#define NO_TOKEN ((size_t)-1)

static bool tok_is(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

static bool ident(const KAst *ast, size_t i) {
    return i < ast->tokens.len && k_token_is_ident(k_em_tok(ast, i));
}

/* whether the island's detail is `<verb> → core …` */
static bool is_core_verb(const KAst *ast, const KIsland *is, const char *verb) {
    if (is->kind != K_ISLAND_CALL) return false;
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, arrow = len;
    for (size_t k = 0; k + 3 <= len; k++)
        if (memcmp(d + k, ARROW, 3) == 0) { arrow = k; break; }
    if (arrow == len) return false;
    size_t s = arrow + 3;
    while (s < len && d[s] == ' ') s++;
    if (len - s < 4 || memcmp(d + s, "core", 4) != 0) return false;
    size_t v = strlen(verb);
    return arrow > v && memcmp(d, verb, v) == 0 && d[v] == '/';
}

/* The module `array` the token `tok` names: its node, the tokens of its element
 * type [type_a, type_b), and whether every dimension is a literal. */
static const KAstNode *array_decl(const KAst *ast, size_t tok, size_t *type_a, size_t *type_b) {
    KToken t = k_em_tok(ast, tok);
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_VARIABLE || n->name_first >= n->name_end) continue;
        KToken nm = k_em_tok(ast, n->name_first);
        if (nm.len != t.len || memcmp(nm.ptr, t.ptr, t.len) != 0) continue;
        size_t i = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
        if (!tok_is(ast, i, "array")) return NULL;
        /* `array i32 v[6]`: the type is what stands between `array` and the name */
        if (n->name_first <= i + 1) return NULL;
        *type_a = i + 1;
        *type_b = n->name_first;
        /* every dimension a literal: `sizeof` is the extent only then */
        if (!tok_is(ast, n->name_first + 1, "[")) return NULL;
        for (size_t p = n->name_first + 2; p < n->end && !tok_is(ast, p, "="); p++) {
            if (tok_is(ast, p, "]") || tok_is(ast, p, "[") || tok_is(ast, p, ",")) continue;
            if (tok_is(ast, p, ";")) break;
            KToken d = k_em_tok(ast, p);
            if (d.len == 0 || d.ptr[0] < '0' || d.ptr[0] > '9') return NULL;
        }
        return n;
    }
    return NULL;
}

/* whether the module declares a name any token of [a, b) spells */
static bool names_declaration(const KAst *ast, size_t a, size_t b) {
    for (size_t i = a; i < b; i++) {
        if (!ident(ast, i)) continue;
        KToken t = k_em_tok(ast, i);
        for (size_t k = 0; k < ast->nodes.len; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            if (n->name_first >= n->name_end) continue;
            KToken nm = k_em_tok(ast, n->name_first);
            if (nm.len == t.len && memcmp(nm.ptr, t.ptr, t.len) == 0) return true;
        }
    }
    return false;
}

/* `keel.length(v)` with one argument: the tokens of the call, and the element
 * type of the array it names. */
static bool length_at(const KAst *ast, const KIsland *is, size_t *open, size_t *arg, size_t *close,
                      size_t *type_a, size_t *type_b) {
    if (!is_core_verb(ast, is, "keel.length")) return false;
    size_t i = is->anchor;
    if (!tok_is(ast, i, "keel") || !tok_is(ast, i + 1, ".") || !tok_is(ast, i + 2, "length")) return false;
    if (!tok_is(ast, i + 3, "(") || !ident(ast, i + 4) || !tok_is(ast, i + 5, ")")) return false;
    if (!array_decl(ast, i + 4, type_a, type_b)) return false;
    if (names_declaration(ast, *type_a, *type_b)) return false;
    *open = i + 3;
    *arg = i + 4;
    *close = i + 5;
    return true;
}

bool k_em_handles_core(const KAst *ast, const KIsland *is) {
    size_t open, arg, close, type_a, type_b;
    return length_at(ast, is, &open, &arg, &close, &type_a, &type_b);
}

void k_em_edits_core(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->anchor < n->first || is->anchor >= n->end) continue;
        size_t open, arg, close, type_a, type_b;
        if (!length_at(ast, is, &open, &arg, &close, &type_a, &type_b)) continue;
        /* `keel.length(` -> `sizeof(` */
        size_t at = k_em_tok_off(ast, is->anchor);
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at,
                .skip = k_em_tok_end(ast, open) - at, .text = "sizeof(", .text_len = 7 });
        /* `)` -> `)/sizeof(<element type>)` */
        size_t ta = k_em_tok_off(ast, type_a), tb = k_em_tok_end(ast, type_b - 1);
        at = k_em_tok_off(ast, close);
        if (at >= a && at < b) {
            char *w = k_em_scratch(e, (tb - ta) + 11);
            if (!w) return;
            size_t q = 0;
            memcpy(w + q, ")/sizeof(", 9); q += 9;
            memcpy(w + q, ast->source.ptr + ta, tb - ta); q += tb - ta;
            w[q++] = ')';
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 1, .text = w, .text_len = q });
        }
    }
}
