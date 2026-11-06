/* engine/emit/uses.c — the uses of another module's names (backend §4.3.2,
 * rules 4 and 7; linguagem §4.4). The parser resolved every use into an
 * island whose detail is `<written> → <symbol> [marks]`:
 *
 *   type  `stack i32 → coll_stack_i32`             the type, by the instance's name
 *   name  `coll.EMPTY → coll_EMPTY`                a constant or variable of a module
 *   call  `length/1 → coll_stack_i32_length &1`    a verb; `&k`: argument k by address
 *
 * A use whose name the module declares itself is renamed by the lexical rule of
 * emit.c (k_em_declared_at); the rest, the foreign uses, are renamed here: the
 * written tokens are replaced by the symbol, and `&` goes before each argument
 * a mark names. The .c then includes the .h of each module and instance it
 * uses (rule 4): the imported modules in the order of the imports, then the
 * instances in the order of the closure. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/names.h"
#include "engine/symtab.h"

#define NO_TOKEN ((size_t)-1)
#define ARROW "\xE2\x86\x92"

typedef struct {
    const char *left; size_t left_len;      /* the written name: "stack i32", "coll.EMPTY", "length" */
    const char *sym; size_t sym_len;        /* the symbol */
    const char *marks; size_t marks_len;    /* after the symbol: " &1", or empty */
} Detail;

static bool detail_of(const KAst *ast, const KIsland *is, Detail *d) {
    const char *t = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, arrow = len;
    for (size_t k = 0; k + 3 <= len; k++)
        if (memcmp(t + k, ARROW, 3) == 0) { arrow = k; break; }
    if (arrow == len || arrow == 0 || arrow + 4 > len) return false;
    size_t l = arrow;
    while (l > 0 && t[l - 1] == ' ') l--;
    if (is->kind == K_ISLAND_CALL) {
        size_t s = 0;
        while (s < l && t[s] != '/') s++;
        l = s;
    }
    d->left = t; d->left_len = l;
    size_t s = arrow + 3;
    while (s < len && t[s] == ' ') s++;
    size_t e = s;
    while (e < len && t[e] != ' ') e++;
    d->sym = t + s; d->sym_len = e - s;
    d->marks = t + e; d->marks_len = len - e;
    return d->sym_len > 0 && d->left_len > 0;
}

/* The tokens the written name spans, from the anchor: each token must be the
 * next bytes of the name (blanks in the name are skipped). The end token, or
 * NO_TOKEN. */
static size_t span_end(const KAst *ast, size_t anchor, const Detail *d) {
    size_t q = 0, i = anchor;
    while (q < d->left_len) {
        if (d->left[q] == ' ') { q++; continue; }
        if (i >= ast->tokens.len) return NO_TOKEN;
        KToken t = k_em_tok(ast, i);
        if (t.len == 0 || q + t.len > d->left_len || memcmp(d->left + q, t.ptr, t.len) != 0) return NO_TOKEN;
        q += t.len;
        i++;
    }
    return i;
}

/* whether the module declares the written name itself (the rule of emit.c) */
static bool declared_here(const KAst *ast, const Detail *d) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->name_first >= n->name_end) continue;
        KToken t = k_em_tok(ast, n->name_first);
        size_t nm = 0;
        while (nm < d->left_len && d->left[nm] != ' ') nm++;
        if (t.len == nm && memcmp(t.ptr, d->left, nm) == 0) return true;
    }
    return false;
}

/* The marks are `&<k>` (argument k by address), `*<k>` (argument k, a value
 * parameter given a pointer, by dereference: spec §4.4, rule 5) and `type:<k>`
 * (argument k is a type, erased into its size and alignment: backend §5.16); out:
 * a bit per argument (1-based) for each. */
static bool marks_of3(const Detail *d, unsigned *amp, unsigned *type, unsigned *dim, unsigned *deref);

/* the four marks; `dim:<k>`: argument k is an `array`, and its dimension 0
 * goes after it as an argument (backend §5.2, §5.18) */
static bool marks_of3(const Detail *d, unsigned *amp, unsigned *type, unsigned *dim, unsigned *deref) {
    *amp = 0;
    *type = 0;
    *dim = 0;
    *deref = 0;
    size_t p = 0;
    while (p < d->marks_len) {
        if (d->marks[p] == ' ') { p++; continue; }
        unsigned *set = amp;
        if (d->marks[p] == '&') p++;
        else if (d->marks[p] == '*') { p++; set = deref; }
        else if (p + 5 <= d->marks_len && memcmp(d->marks + p, "type:", 5) == 0) { p += 5; set = type; }
        else if (p + 4 <= d->marks_len && memcmp(d->marks + p, "dim:", 4) == 0) { p += 4; set = dim; }
        else return false;
        unsigned k = 0;
        size_t digits = 0;
        while (p < d->marks_len && d->marks[p] >= '0' && d->marks[p] <= '9') { k = k * 10 + (unsigned)(d->marks[p++] - '0'); digits++; }
        if (!digits || k == 0 || k > 16) return false;
        *set |= 1u << k;
    }
    return true;
}

/* The first token of each argument of the call whose `(` is `open`; false when
 * the call is not well formed. */
static bool call_args(const KAst *ast, size_t open, size_t first[17], size_t last[17], size_t *argc) {
    if (open >= ast->tokens.len || !k_em_text_is(k_em_tok(ast, open), "(")) return false;
    size_t depth = 0, start = open + 1;
    *argc = 0;
    for (size_t j = open + 1; j < ast->tokens.len; j++) {
        KToken t = k_em_tok(ast, j);
        if (k_em_text_is(t, "(") || k_em_text_is(t, "[") || k_em_text_is(t, "{")) depth++;
        else if (depth > 0 && (k_em_text_is(t, ")") || k_em_text_is(t, "]") || k_em_text_is(t, "}"))) depth--;
        else if (depth == 0 && (k_em_text_is(t, ",") || k_em_text_is(t, ")"))) {
            if (j > start) {
                if (*argc == 16) return false;
                first[++*argc] = start;
                last[*argc] = j;
            }
            start = j + 1;
            if (k_em_text_is(t, ")")) return true;
        }
    }
    return false;
}

/* A call of the module's own verb that is more than a rename: an erased `type`
 * argument or a dimension (marks), or an arity suffix on the symbol (backend
 * §2.1.1, §5.16). It is rewritten whole, as a call of another module is. */
static bool own_call_rewritten(const Detail *d) {
    for (size_t k = 0; k + 5 <= d->marks_len; k++)
        if (memcmp(d->marks + k, "type:", 5) == 0) return true;
    for (size_t k = 0; k + 4 <= d->marks_len; k++)
        if (memcmp(d->marks + k, "dim:", 4) == 0) return true;
    size_t n = d->sym_len;
    while (n > 0 && d->sym[n - 1] >= '0' && d->sym[n - 1] <= '9') n--;
    return n < d->sym_len && n >= d->left_len && memcmp(d->sym + n - d->left_len, d->left, d->left_len) == 0;
}

bool k_em_foreign_island(const KAst *ast, const KIsland *is) {
    if (is->kind != K_ISLAND_TYPE && is->kind != K_ISLAND_NAME && is->kind != K_ISLAND_CALL) return false;
    Detail d;
    if (!detail_of(ast, is, &d)) return false;
    /* a generic's tree has islands only as one of its instances reads it: the type
       and the call carry the instance's symbol (k_collect_instance_islands) */
    if (k_em_module_is_generic(ast) && (is->kind == K_ISLAND_TYPE || is->kind == K_ISLAND_CALL)) return true;
    if (!declared_here(ast, &d)) return true;
    return is->kind == K_ISLAND_CALL && own_call_rewritten(&d);
}

/* The module a callee names: `M.f` by the alias M, `f` by the symbol table. */
static const KAst *callee_module(const KAst *ast, size_t anchor, size_t end, size_t *name) {
    if (!ast->symbols) return NULL;
    *name = end - 1;
    if (end - anchor == 3 && k_em_text_is(k_em_tok(ast, anchor + 1), ".")) {
        KToken m = k_em_tok(ast, anchor);
        for (size_t i = 0; i < ast->symbols->len; i++) {
            const KSymbol *s = keel_buffer_KSymbol_ptr(ast->symbols, i);
            if (s->kind == K_SYM_MODULE && s->origin && s->origin->ast &&
                k_symtab_same_name(s->name, (keel_slice_char){ m.len, (char *)m.ptr }))
                return s->origin->ast;
        }
        return NULL;
    }
    if (end - anchor != 1) return NULL;
    KToken f = k_em_tok(ast, anchor);
    const KSymbol *s = k_symbol_resolve(ast->symbols, (keel_slice_char){ f.len, (char *)f.ptr });
    if (s && s->kind == K_SYM_FUNCTION && s->origin) return s->origin->ast;
    for (size_t k = 0; k < ast->nodes.len; k++) {   /* a verb of this module */
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind == K_AST_FUNCTION && n->name_first < n->name_end) {
            KToken t = k_em_tok(ast, n->name_first);
            if (t.len == f.len && memcmp(t.ptr, f.ptr, f.len) == 0) return ast;
        }
    }
    return NULL;
}

/* Whether the verb `name` of module `m` with `argc` parameters returns `X *`,
 * X the `type X` parameter k (backend §5.16, rule 2: the call gets the cast).
 * 1 yes, 0 no, -1 the declaration was not found. */
static int returns_type_pointer(const KAst *m, KToken name, size_t argc, size_t k) {
    for (size_t i = 0; i < m->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&m->nodes, i);
        if (n->kind != K_AST_FUNCTION || n->name_first >= n->name_end || n->name_first < 2) continue;
        KToken t = k_em_tok(m, n->name_first);
        if (t.len != name.len || memcmp(t.ptr, name.ptr, t.len) != 0) continue;
        size_t open = n->name_first + 1;
        if (open >= n->end || !k_em_text_is(k_em_tok(m, open), "(")) continue;
        /* the parameters: their first tokens, top-level */
        size_t pf[17], count = 0, depth = 0, start = open + 1;
        for (size_t j = open + 1; j < n->end; j++) {
            KToken x = k_em_tok(m, j);
            if (k_em_text_is(x, "(") || k_em_text_is(x, "[")) depth++;
            else if (depth > 0 && (k_em_text_is(x, ")") || k_em_text_is(x, "]"))) depth--;
            else if (depth == 0 && (k_em_text_is(x, ",") || k_em_text_is(x, ")"))) {
                if (j > start && !(j == start + 1 && k_em_text_is(k_em_tok(m, start), "void"))) {
                    if (count == 16) break;
                    pf[++count] = start;
                }
                start = j + 1;
                if (k_em_text_is(x, ")")) break;
            }
        }
        if (count != argc || k > count) continue;
        if (!k_em_text_is(k_em_tok(m, pf[k]), "type")) continue;
        KToken x = k_em_tok(m, pf[k] + 1);
        bool ptr = k_em_text_is(k_em_tok(m, n->name_first - 1), "*");
        KToken r = k_em_tok(m, n->name_first - 2);
        return ptr && r.len == x.len && memcmp(r.ptr, x.ptr, x.len) == 0;
    }
    return -1;
}

/* The arguments of a call of verb `name` of module `m` with `argc` parameters
 * that select the instance (backend §5.16, rule 3): the parameter is `type X`,
 * with X a type binder of `m`, and the argument leaves the call. A bit per
 * argument, 1-based; 0 when there is none or the declaration is not found. */
static unsigned selection_mask(const KAst *m, KToken name, size_t argc) {
    const KAstNode *mn = keel_buffer_KAstNode_ptr(&m->nodes, m->module);
    unsigned mask = 0;
    for (size_t i = 0; i < m->nodes.len && !mask; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&m->nodes, i);
        if (n->kind != K_AST_FUNCTION || n->name_first >= n->name_end) continue;
        KToken t = k_em_tok(m, n->name_first);
        if (t.len != name.len || memcmp(t.ptr, name.ptr, t.len) != 0) continue;
        size_t open = n->name_first + 1;
        if (open >= n->end || !k_em_text_is(k_em_tok(m, open), "(")) continue;
        size_t pf[17], count = 0, depth = 0, start = open + 1;
        for (size_t j = open + 1; j < n->end; j++) {
            KToken x = k_em_tok(m, j);
            if (k_em_text_is(x, "(") || k_em_text_is(x, "[")) depth++;
            else if (depth > 0 && (k_em_text_is(x, ")") || k_em_text_is(x, "]"))) depth--;
            else if (depth == 0 && (k_em_text_is(x, ",") || k_em_text_is(x, ")"))) {
                if (j > start && count < 16) pf[++count] = start;
                start = j + 1;
                if (k_em_text_is(x, ")")) break;
            }
        }
        if (count != argc) continue;
        for (size_t k = 1; k <= count; k++) {
            if (!k_em_text_is(k_em_tok(m, pf[k]), "type")) continue;
            KToken x = k_em_tok(m, pf[k] + 1);
            for (size_t b = mn->type_first; b < mn->type_end; b++) {
                KToken y = k_em_tok(m, b);
                if (k_token_is_ident(y) && y.len == x.len && memcmp(y.ptr, x.ptr, x.len) == 0) mask |= 1u << k;
            }
        }
    }
    return mask;
}

/* whether a foreign island anchors in the tokens [a, b) */
static bool foreign_in(const KAst *ast, size_t a, size_t b) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->anchor >= a && is->anchor < b && k_em_foreign_island(ast, is)) return true;
    }
    return false;
}

/* The dimension 0 of the `array` named by the single token `tok`, as its
 * declaration writes it (a decimal), from the array island `name [d0, ...]`. */
static bool array_dim0(const KAst *ast, size_t tok, const char **p, size_t *len, bool *binder) {
    if (binder) *binder = false;
    KToken t = k_em_tok(ast, tok);
    /* the declaration the use sees: the nearest one before it, in its function (two
     * functions may name their tables alike, `steps[3]` and `steps[2]`) */
    const KIsland *best = NULL;
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->kind != K_ISLAND_ARRAY || is->anchor > tok || (best && is->anchor < best->anchor)) continue;
        const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
        if (is->text_len < t.len + 3 || memcmp(d, t.ptr, t.len) != 0 || d[t.len] != ' ' || d[t.len + 1] != '[') continue;
        bool same_fn = true;
        for (size_t k = 0; k < ast->nodes.len && same_fn; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            if (n->kind == K_AST_FUNCTION && is->anchor >= n->first && is->anchor < n->end) same_fn = tok >= n->first && tok < n->end;
        }
        if (same_fn) best = is;
    }
    if (!best) return false;
    {
        const KIsland *is = best;
        const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
        size_t s = t.len + 2, e = s;
        while (e < is->text_len && d[e] >= '0' && d[e] <= '9') e++;
        if (e == s) {
            /* a parameter with a binder, `[size_t n]`: the dimension that arrived is
               its name, which the C spells keel__n (backend §5.18, rule 2) */
            size_t w = s;
            while (w < is->text_len && d[w] != ']' && d[w] != ',') w++;
            size_t n1 = w;
            while (n1 > s && d[n1 - 1] != ' ') n1--;
            if (w >= is->text_len || n1 == s || n1 == w) return false;
            *p = d + n1;
            *len = w - n1;
            if (binder) *binder = true;
            return true;
        }
        if (e >= is->text_len || (d[e] != ']' && d[e] != ',')) return false;
        *p = d + s;
        *len = e - s;
        return true;
    }
}

bool k_em_array_dim0(const KAst *ast, size_t tok, const char **p, size_t *len, bool *binder) {
    return array_dim0(ast, tok, p, len, binder);
}

bool k_em_handles_use(const KAst *ast, const KIsland *is) {
    Detail d;
    if (!k_em_foreign_island(ast, is) || !detail_of(ast, is, &d)) return false;
    if (d.sym_len == 4 && memcmp(d.sym, "core", 4) == 0) return false;   /* lowered by the core: not a rename */
    unsigned amp, type, dim, deref;
    if (!marks_of3(&d, &amp, &type, &dim, &deref)) return false;
    size_t end = span_end(ast, is->anchor, &d);
    if (end == NO_TOKEN) return false;
    if (is->kind != K_ISLAND_CALL) return amp == 0 && type == 0 && dim == 0 && deref == 0;
    size_t first[17], last[17], argc;
    if (!call_args(ast, end, first, last, &argc)) return false;
    for (unsigned k = 1; k <= 16; k++) {
        if (((amp | type | dim | deref) & (1u << k)) && k > argc) return false;
        const char *dp;
        size_t dl;
        if ((dim & (1u << k)) && (last[k] != first[k] + 1 || !array_dim0(ast, first[k], &dp, &dl, NULL))) return false;
        if (!(type & (1u << k))) continue;
        /* the type argument is copied twice: it must not hold another rename */
        if (foreign_in(ast, first[k], last[k])) return false;
        size_t name;
        const KAst *m = callee_module(ast, is->anchor, end, &name);
        if (!m || returns_type_pointer(m, k_em_tok(ast, name), argc, k) < 0) return false;
    }
    return true;
}

/* the implicit `= {0}` of an `arena` defined without initializer (backend §5.2,
 * rule 8): the island's anchor is the declarator's name */
bool k_em_handles_implicit_init(const KAst *ast, const KIsland *is) {
    if (is->kind != K_ISLAND_IMPLICIT_INIT || is->anchor + 1 >= ast->tokens.len) return false;
    KToken next = k_em_tok(ast, is->anchor + 1);
    return k_em_text_is(next, ";") || k_em_text_is(next, ",");
}

/* `ref` in a declaration (spec §4.12) is the language's: the word goes, with
 * the blanks after it */
bool k_em_handles_ref(const KAst *ast, const KIsland *is) {
    return is->kind == K_ISLAND_REF && is->anchor + 1 < ast->tokens.len && k_em_text_is(k_em_tok(ast, is->anchor), "ref");
}

/* removes the edits whose position is in [lo, hi) (a span copied again) */
static void drop_edits(Edit *ed, size_t *cnt, size_t lo, size_t hi) {
    size_t j = 0;
    for (size_t k = 0; k < *cnt; k++)
        if (ed[k].at < lo || ed[k].at >= hi) ed[j++] = ed[k];
    *cnt = j;
}

/* A type or a constant of the generic that the instance declares (backend §4.3, rule 1):
 * the module's name for it is the generic's, but it lives in the instance's .type.h, and
 * the instance's name is written by decls.c. */
bool k_em_own_declaration(const Emit *e, const char *sym, size_t len) {
    if (!e->inst) return false;
    for (size_t k = 0; k < e->ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, k);
        if ((n->kind != K_AST_TYPE && n->kind != K_AST_CONSTEXPR) || n->name_first >= n->name_end) continue;
        if (!k_em_generic_out(e->ast, n)) continue;
        KToken t = k_em_tok(e->ast, n->name_first);
        char m[NAME_CAP];
        size_t ml = k_mangle_decl(e->module, (keel_slice_char){ t.len, (char *)t.ptr }, false, (keel_slice_char){ sizeof m, m });
        if (ml <= sizeof m && ml == len && memcmp(m, sym, len) == 0) return true;
    }
    return false;
}

void k_em_edits_uses(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    const char *align = e->opt.c23 ? "alignof(" : "_Alignof(";
    for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->anchor < n->first || is->anchor >= n->end) continue;
        if (k_em_parallel_interrupted(e, is, a, b, ed, cnt)) continue;
        if (k_em_handles_implicit_init(ast, is)) {
            size_t at = k_em_tok_end(ast, is->anchor);
            if (at > a && at <= b)
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0, .text = " = {0}", .text_len = 6 });
            continue;
        }
        if (k_em_handles_ref(ast, is)) {
            size_t at = k_em_tok_off(ast, is->anchor), to = k_em_tok_off(ast, is->anchor + 1);
            if (at >= a && at < b)
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at, .skip = to - at });
            continue;
        }
        if (!k_em_handles_use(ast, is)) {
            /* a call of the module's own verb: the name is renamed by emit.c, the `&` are here */
            Detail o;
            unsigned amp, type, dim, deref;
            size_t f[17], l[17], n_args, end;
            if (is->kind != K_ISLAND_CALL || k_em_foreign_island(ast, is) || !detail_of(ast, is, &o) ||
                !marks_of3(&o, &amp, &type, &dim, &deref) || dim || (amp | deref) == 0 ||
                (end = span_end(ast, is->anchor, &o)) == NO_TOKEN || !call_args(ast, end, f, l, &n_args)) continue;
            for (size_t k = 1; k <= n_args; k++) {
                size_t p = k_em_tok_off(ast, f[k]);
                if ((amp & (1u << k)) && p >= a && p < b)
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = 0, .text = "&", .text_len = 1 });
                if ((deref & (1u << k)) && p >= a && p < b)
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = 0, .text = "*", .text_len = 1 });
            }
            continue;
        }
        Detail d;
        detail_of(ast, is, &d);
        if (is->kind == K_ISLAND_TYPE && k_em_own_declaration(e, d.sym, d.sym_len)) continue;   /* decls.c names it */
        size_t end = span_end(ast, is->anchor, &d);
        size_t at = k_em_tok_off(ast, is->anchor);
        unsigned amp = 0, type = 0, dim = 0, deref = 0;
        size_t first[17], last[17], argc = 0;
        unsigned sel = 0;
        if (is->kind == K_ISLAND_CALL) {
            marks_of3(&d, &amp, &type, &dim, &deref);
            call_args(ast, end, first, last, &argc);
            size_t name;
            const KAst *m = callee_module(ast, is->anchor, end, &name);
            if (m && m != ast) sel = selection_mask(m, k_em_tok(ast, name), argc);
        }
        if (at >= a && at < b) {
            /* the written name is replaced whole: a rename inside it (`grid(DIM) f32`) goes */
            drop_edits(ed, cnt, at, k_em_tok_end(ast, end - 1));
            /* the cast of a `X *` result, before the call (backend §5.16, rule 2) */
            for (size_t k = 1; k <= argc; k++) {
                if (!(type & (1u << k))) continue;
                size_t name;
                const KAst *m = callee_module(ast, is->anchor, end, &name);
                if (m && returns_type_pointer(m, k_em_tok(ast, name), argc, k) == 1) {
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0, .text = "(", .text_len = 1 });
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TOKS, .at = at, .skip = 0, .tok_a = first[k], .tok_b = last[k] });
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0, .text = " *)", .text_len = 3 });
                }
            }
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok_end(ast, end - 1) - at,
                .text = d.sym, .text_len = d.sym_len });
        }
        for (size_t k = 1; k <= argc; k++) {
            size_t p = k_em_tok_off(ast, first[k]), q = k_em_tok_end(ast, last[k] - 1);
            if (p < a || p >= b) continue;
            if (sel & (1u << k)) {
                /* a selecting type leaves the call, with its comma (backend §5.16, rule 3) */
                size_t from = p, to = k < argc ? k_em_tok_off(ast, first[k + 1]) : q;
                if (k == argc && k > 1) from = k_em_tok_end(ast, last[k - 1] - 1);
                if (to > b) { e->ok = false; return; }
                drop_edits(ed, cnt, from, to);
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = from, .skip = to - from });
                continue;
            }
            if (amp & (1u << k))
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = 0, .text = "&", .text_len = 1 });
            if (deref & (1u << k))
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = 0, .text = "*", .text_len = 1 });
            if (dim & (1u << k)) {
                /* the array's dimension 0, from the declaration, as one argument more */
                const char *dp;
                size_t dl;
                bool bound;
                if (!array_dim0(ast, first[k], &dp, &dl, &bound) || q > b) { e->ok = false; return; }
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = q, .skip = 0, .text = ", ", .text_len = 2 });
                if (bound) k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = q, .skip = 0, .text = "keel__", .text_len = 6 });
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = q, .skip = 0, .text = dp, .text_len = dl });
            }
            if (type & (1u << k)) {
                /* T -> sizeof(T), alignof(T): the type is copied twice, with its renames */
                if (q > b) { e->ok = false; return; }
                drop_edits(ed, cnt, p, q);
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = 0, .text = "sizeof(", .text_len = 7 });
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TOKS, .at = p, .skip = q - p, .tok_a = first[k], .tok_b = last[k] });
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = q, .skip = 0, .text = "), ", .text_len = 3 });
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = q, .skip = 0, .text = align, .text_len = strlen(align) });
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TOKS, .at = q, .skip = 0, .tok_a = first[k], .tok_b = last[k] });
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = q, .skip = 0, .text = ")", .text_len = 1 });
            }
        }
    }
    /* C11 spells the C23 keywords of alignment with an underscore (backend §9.1) */
    if (!e->opt.c23)
        for (size_t i = n->first; i < n->end && e->ok; i++) {
            KToken t = k_em_tok(ast, i);
            size_t at = k_em_tok_off(ast, i);
            if (at < a || at >= b) continue;
            bool taken = false;   /* already rewritten whole (an erased `alignof(T)`) */
            for (size_t k = 0; k < *cnt; k++) if (ed[k].at == at && ed[k].skip > t.len) taken = true;
            if (taken) continue;
            if (k_em_text_is(t, "alignof"))
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = t.len, .text = "_Alignof", .text_len = 8 });
            else if (k_em_text_is(t, "alignas"))
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = t.len, .text = "_Alignas", .text_len = 8 });
        }
    k_em_sort_edits(ed, 0, *cnt);
}

/* The symbol of a type spelled `spelled` ("buffer i32"): the one a type island
 * with that written name resolved to. */
bool k_em_type_symbol(const KAst *ast, const char *spelled, size_t len, const char **sym, size_t *sym_len) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        Detail d;
        if (is->kind != K_ISLAND_TYPE || !detail_of(ast, is, &d)) continue;
        if (d.left_len == len && memcmp(d.left, spelled, len) == 0) { *sym = d.sym; *sym_len = d.sym_len; return true; }
    }
    return false;
}

/* the symbol of the type island anchored at token `at` (a type written in a signature): false
 * when no type island starts there */
bool k_em_type_island_at(const KAst *ast, size_t at, const char **sym, size_t *sym_len) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        Detail d;
        if (is->kind != K_ISLAND_TYPE || is->anchor != at || !detail_of(ast, is, &d)) continue;
        *sym = d.sym; *sym_len = d.sym_len;
        return true;
    }
    return false;
}

bool k_em_type_by_value(const KAst *ast, const char *sym, size_t len) {
    for (size_t c = 0; c < ast->closure.len; c++) {
        const KClosure *k = keel_buffer_KClosure_ptr(&ast->closure, c);
        if (k->kind != K_CLOSURE_INSTANCE || k->symbol_len != len || memcmp(k->symbol, sym, len) != 0 || !k->module || !k->module->ast) continue;
        const KAst *g = k->module->ast;
        for (size_t q = 0; q < g->nodes.len; q++) {
            const KAstNode *m = keel_buffer_KAstNode_ptr(&g->nodes, q);
            if (m->kind != K_AST_MODIFIER) continue;
            for (size_t j = m->name_first; j < m->end; j++)
                if (k_em_text_is(k_em_tok(g, j), "byref")) return false;
            return true;
        }
    }
    return false;
}

/* For k_em_put_range, the copy by tokens: the use anchored at token i. 1 with
 * the symbol to write instead of tokens [i, end) (none for a call of the
 * module's own verb, which emit.c renames) and the first tokens of the
 * arguments that take `&`; 0 when there is no use there; -1 when the use needs
 * more than a rename (a `type:` or `dim:` argument). */
int k_em_range_use(const KAst *ast, size_t i, KRangeUse *r) {
    for (size_t k = 0; k < ast->islands.len; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->anchor != i || (is->kind != K_ISLAND_TYPE && is->kind != K_ISLAND_NAME && is->kind != K_ISLAND_CALL)) continue;
        Detail d;
        if (!detail_of(ast, is, &d)) continue;
        bool foreign = k_em_handles_use(ast, is);
        if (!foreign && (is->kind != K_ISLAND_CALL || k_em_foreign_island(ast, is))) continue;
        unsigned amp, type, dim, deref;
        if (!marks_of3(&d, &amp, &type, &dim, &deref)) return -1;
        if (type || dim) return -1;
        size_t end = span_end(ast, is->anchor, &d);
        if (end == NO_TOKEN) return -1;
        *r = (KRangeUse){ .sym = foreign ? d.sym : NULL, .sym_len = foreign ? d.sym_len : 0, .end = end };
        if (is->kind == K_ISLAND_CALL && (amp | deref)) {
            size_t first[17], last[17], argc;
            if (!call_args(ast, end, first, last, &argc)) return -1;
            for (size_t a = 1; a <= argc; a++) {
                if (amp & (1u << a)) r->amp[r->namp++] = first[a];
                if (deref & (1u << a)) r->star[r->nstar++] = first[a];
            }
        }
        return 1;
    }
    return 0;
}

/* ---- the .h: another module's types in the signatures (rule 2) ---- */

/* `typedef struct X X;` for each type of another module (an instance, a
 * struct) a public signature takes by pointer, once, in the order they appear;
 * by value it would need the #include of its .type.h, which is not here yet:
 * refused. */
void k_em_h_forward_decls(Emit *e) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->nodes.len && e->ok; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_FUNCTION || !n->is_public || n->body_first == NO_TOKEN || n->body_first < 1 || k_em_skip(e, n)) continue;
        for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
            const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
            if (is->kind != K_ISLAND_TYPE || is->anchor < n->first || is->anchor >= n->body_first - 1) continue;
            if (!k_em_handles_use(ast, is)) continue;
            Detail d;
            detail_of(ast, is, &d);
            size_t end = span_end(ast, is->anchor, &d);
            if (end >= ast->tokens.len || !k_em_text_is(k_em_tok(ast, end), "*")) continue;   /* by value: k_em_h_type_includes */
            if (k_em_sym_is_tags_named(ast, d.sym, d.sym_len, d.left, d.left_len)) continue;   /* an enum: its .type.h, see k_em_h_type_includes */
            if (k_em_own_instance(e, d.sym, d.sym_len)) continue;
            char own[NAME_CAP], hp[NAME_CAP];
            size_t ol = k_em_own_type_path(e, own, sizeof own);
            size_t hl = ol ? k_em_header_of(ast, d.sym, d.sym_len, ".type.h", hp, sizeof hp) : 0;
            if (ol && hl == ol && memcmp(hp, own, ol) == 0) continue;
            /* once: not if an earlier island of a public signature has the same symbol */
            bool seen = false;
            for (size_t j = 0; j < i && !seen; j++) {
                const KIsland *o = keel_buffer_KIsland_ptr(&ast->islands, j);
                Detail od;
                if (o->kind != K_ISLAND_TYPE || !k_em_handles_use(ast, o) || !detail_of(ast, o, &od)) continue;
                size_t oe = span_end(ast, o->anchor, &od);
                if (oe >= ast->tokens.len || !k_em_text_is(k_em_tok(ast, oe), "*")) continue;
                const KAstNode *on = NULL;
                for (size_t q = 0; q < ast->nodes.len; q++) {
                    const KAstNode *x = keel_buffer_KAstNode_ptr(&ast->nodes, q);
                    if (x->kind == K_AST_FUNCTION && x->is_public && x->body_first != NO_TOKEN &&
                        o->anchor >= x->first && o->anchor < x->body_first - 1) on = x;
                }
                seen = on && od.sym_len == d.sym_len && memcmp(od.sym, d.sym, d.sym_len) == 0;
            }
            if (seen) continue;
            k_emit_raw(&e->w, (keel_slice_char){ 15, (char *)"typedef struct " });
            k_emit_raw(&e->w, (keel_slice_char){ d.sym_len, (char *)d.sym });
            k_emit_raw(&e->w, (keel_slice_char){ 1, (char *)" " });
            k_emit_raw(&e->w, (keel_slice_char){ d.sym_len, (char *)d.sym });
            k_emit_raw(&e->w, (keel_slice_char){ 2, (char *)";\n" });
        }
    }
}

/* .h, section 1: the .type.h of each type of another module a public signature
 * takes by value, in the order of the imports, and of the first use within one
 * (codegen design E2: the include stands for the import) */
void k_em_h_type_includes(Emit *e) {
    const KAst *ast = e->ast;
    Includes s = { .n = 0 };
    size_t rank[MAX_INCLUDES];
    char own[NAME_CAP];
    size_t ol = k_em_own_type_path(e, own, sizeof own);
    if (ol && !k_em_add_include(&s, own, ol)) { e->ok = false; return; }
    size_t first = s.n;   /* the own .type.h is written by emit.c */
    if (e->inst && e->inst->proto_fn.len && !e->inst->proto_array) {
        /* the concrete type of a function over a protocol, ahead (backend §5.19, rule 4) */
        char cp[NAME_CAP];
        size_t cl = k_mangle_instance_path(e->inst->proto_module, e->inst->proto_concrete, ".type.h", (keel_slice_char){ sizeof cp, cp });
        if (cl > sizeof cp || !k_em_add_include(&s, cp, cl)) { e->ok = false; return; }
        rank[s.n - 1] = 0;
    }
    for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->kind != K_ISLAND_TYPE || !k_em_handles_use(ast, is)) continue;
        bool sig = false;
        for (size_t k = 0; k < ast->nodes.len && !sig; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            sig = n->kind == K_AST_FUNCTION && n->is_public && n->body_first != NO_TOKEN && n->body_first >= 1 &&
                  is->anchor >= n->first && is->anchor < n->body_first - 1 && !k_em_skip(e, n);
        }
        if (!sig) continue;
        Detail d;
        detail_of(ast, is, &d);
        size_t end = span_end(ast, is->anchor, &d);
        if (end < ast->tokens.len && k_em_text_is(k_em_tok(ast, end), "*") && !k_em_sym_is_tags_named(ast, d.sym, d.sym_len, d.left, d.left_len)) continue;   /* by pointer: forward */
        if (k_em_own_instance(e, d.sym, d.sym_len)) continue;
        char path[NAME_CAP];
        size_t pl = k_em_arg_header(e, d.sym, d.sym_len, ".type.h", path, sizeof path);
        if (pl == SIZE_MAX) continue;
        if (!pl) pl = k_em_header_of(ast, d.sym, d.sym_len, ".type.h", path, sizeof path);
        if (!pl && k_em_plain_arg(e, d.sym, d.sym_len)) continue;   /* a C type: no header of keel */
        if (!pl) { e->ok = false; return; }
        size_t before = s.n;
        if (!k_em_add_include(&s, path, pl)) { e->ok = false; return; }
        if (s.n > before) rank[s.n - 1] = k_em_import_rank(ast, d.sym, d.sym_len);
    }
    /* a binder bound to a type of the program (`E` = `ag_Cycle`) that a public signature takes by
     * value: no island, the argument's own header; after the islands' (first use is by token) */
    for (size_t k = 0; e->inst && e->inst->user && !e->inst->proto_fn.len && k < ast->nodes.len && e->ok; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_FUNCTION || !n->is_public || n->body_first == NO_TOKEN || n->body_first < 1 || k_em_skip(e, n)) continue;
        for (size_t j = n->first; j < n->body_first - 1 && e->ok; j++) {
            keel_slice_char arg;
            KToken t = k_em_tok(ast, j);
            bool member = j > 0 && (k_em_text_is(k_em_tok(ast, j - 1), ".") || k_em_text_is(k_em_tok(ast, j - 1), "->"));
            if (member || !k_token_is_ident(t) || !k_em_binder_arg(e, t, &arg) || (j + 1 < n->body_first && k_em_text_is(k_em_tok(ast, j + 1), "*"))) continue;
            /* an argument that is an instance is reached through the instance's own .type.h */
            bool inst_arg = false;
            for (size_t q = 0; q < e->inst->argc; q++)
                inst_arg = inst_arg || (e->inst->arg_module[q].len && e->inst->arg[q].len == arg.len && memcmp(e->inst->arg[q].ptr, arg.ptr, arg.len) == 0);
            if (inst_arg) continue;
            char path[NAME_CAP];
            size_t pl = k_em_header_of(e->inst->user, arg.ptr, arg.len, ".type.h", path, sizeof path);
            if (!pl || pl == SIZE_MAX) continue;
            size_t before = s.n;
            if (!k_em_add_include(&s, path, pl)) { e->ok = false; return; }
            if (s.n > before) rank[s.n - 1] = (size_t)-1;
        }
    }
    /* stable: by the import, then by the first use */
    for (size_t i = first + 1; i < s.n; i++)
        for (size_t j = i; j > first && rank[j - 1] > rank[j]; j--) {
            char t[NAME_CAP]; size_t tl = s.len[j], tr = rank[j];
            memcpy(t, s.path[j], tl); memcpy(s.path[j], s.path[j - 1], s.len[j - 1]); s.len[j] = s.len[j - 1]; rank[j] = rank[j - 1];
            memcpy(s.path[j - 1], t, tl); s.len[j - 1] = tl; rank[j - 1] = tr;
        }
    for (size_t k = first; k < s.n; k++) k_em_raw_include(e, s.path[k], s.len[k]);
}

/* .c: the .h of each module and instance the module uses, in the order of the path */
void k_em_use_includes(Emit *e) {
    const KAst *ast = e->ast;
    Includes s = { .n = 0 };
    char self[NAME_CAP];   /* the module's own .h is the first include already (a tag of its own set is a use too) */
    size_t sl = k_mangle_path(e->module, ".h", (keel_slice_char){ sizeof self, self });
    for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        Detail d;
        if (!k_em_handles_use(ast, is) || !detail_of(ast, is, &d)) continue;
        char path[NAME_CAP];
        size_t pl = k_em_header_of(ast, d.sym, d.sym_len, ".h", path, sizeof path);
        if (pl == sl && memcmp(path, self, pl) == 0) continue;
        if (pl && !k_em_add_include(&s, path, pl)) { e->ok = false; return; }
    }
    /* a range over an `array` (subscript.c) calls the verbs of the array's instance, which no
     * closure names: its header is the symbol less `_as_slice` and its arity (`keel_array_i32_as_slice2`
     * -> keel/keel_array_i32.h) */
    for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        Detail d;
        unsigned amp, type, dim, deref;
        if (is->kind != K_ISLAND_RANGE_INDEX || !detail_of(ast, is, &d)) continue;
        /* an open end writes its `core` word before the marks (parser_islands_index.c) */
        if (d.marks_len >= 5 && memcmp(d.marks, " core", 5) == 0 && (d.marks_len == 5 || d.marks[5] == ' ')) {
            d.marks += 5; d.marks_len -= 5;
        }
        if (!marks_of3(&d, &amp, &type, &dim, &deref) || !dim) continue;
        size_t n = d.sym_len;
        while (n > 0 && d.sym[n - 1] >= '0' && d.sym[n - 1] <= '9') n--;
        if (n < 9 || memcmp(d.sym + n - 9, "_as_slice", 9) != 0) continue;
        size_t stem = n - 9;
        char path[NAME_CAP];
        if (5 + stem + 2 > sizeof path) { e->ok = false; return; }
        memcpy(path, "keel/", 5);
        memcpy(path + 5, d.sym, stem);
        memcpy(path + 5 + stem, ".h", 2);
        if (!k_em_add_include(&s, path, 5 + stem + 2)) { e->ok = false; return; }
    }
    /* the control symbol of a `parallel` block is a type of keel.parallel */
    for (size_t i = 0; i < ast->islands.len && e->ok; i++)
        if (k_em_handles_parallel(ast, keel_buffer_KIsland_ptr(&ast->islands, i)) && !k_em_add_include(&s, "keel/keel_parallel.h", 20)) { e->ok = false; return; }
    /* the order of the path: an insertion sort, few includes */
    for (size_t i = 1; i < s.n; i++)
        for (size_t j = i; j > 0; j--) {
            size_t m = s.len[j - 1] < s.len[j] ? s.len[j - 1] : s.len[j];
            int c = memcmp(s.path[j - 1], s.path[j], m);
            if (c < 0 || (c == 0 && s.len[j - 1] <= s.len[j])) break;
            char t[NAME_CAP]; size_t tl = s.len[j];
            memcpy(t, s.path[j], tl); memcpy(s.path[j], s.path[j - 1], s.len[j - 1]); s.len[j] = s.len[j - 1];
            memcpy(s.path[j - 1], t, tl); s.len[j - 1] = tl;
        }
    for (size_t k = 0; k < s.n; k++) k_em_raw_include(e, s.path[k], s.len[k]);
}

/* .h, section 3: the .h of each module and instance a `pub inline` body calls,
 * in the order of the path; not this file's own, nor the instance's own verbs */
bool k_em_h_body_includes(Emit *e) {
    const KAst *ast = e->ast;
    char self[NAME_CAP];
    size_t sl = e->inst ? k_mangle_instance_path(e->inst->module, k_emit_unit(e->inst), ".h", (keel_slice_char){ sizeof self, self })
                        : k_mangle_path(e->module, ".h", (keel_slice_char){ sizeof self, self });
    Includes s = { .n = 0 };
    for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        Detail d;
        if (is->kind != K_ISLAND_CALL || !k_em_handles_use(ast, is) || !detail_of(ast, is, &d)) continue;
        bool body = false;
        for (size_t k = 0; k < ast->nodes.len && !body; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            body = n->kind == K_AST_FUNCTION && n->is_public && n->is_inline && n->body_first != NO_TOKEN &&
                   is->anchor >= n->body_first && is->anchor < n->end && !k_em_skip(e, n);
        }
        if (!body || k_em_own_instance(e, d.sym, d.sym_len)) continue;
        char path[NAME_CAP];
        size_t pl = k_em_header_of(ast, d.sym, d.sym_len, ".h", path, sizeof path);
        if (!pl || (pl == sl && memcmp(path, self, pl) == 0)) continue;
        if (!k_em_add_include(&s, path, pl)) { e->ok = false; return false; }
    }
    for (size_t i = 1; i < s.n; i++)
        for (size_t j = i; j > 0; j--) {
            size_t m = s.len[j - 1] < s.len[j] ? s.len[j - 1] : s.len[j];
            int c = memcmp(s.path[j - 1], s.path[j], m);
            if (c < 0 || (c == 0 && s.len[j - 1] <= s.len[j])) break;
            char t[NAME_CAP]; size_t tl = s.len[j];
            memcpy(t, s.path[j], tl); memcpy(s.path[j], s.path[j - 1], s.len[j - 1]); s.len[j] = s.len[j - 1];
            memcpy(s.path[j - 1], t, tl); s.len[j - 1] = tl;
        }
    for (size_t k = 0; k < s.n; k++) k_em_raw_include(e, s.path[k], s.len[k]);
    return s.n > 0;
}

/* .type.h: the .type.h of each type of another module a field of a struct of
 * the module holds by value (backend §4.3.2, rule 2), in the order of the
 * imports and then of the first use; by pointer it needs none. */
void k_em_type_h_includes(Emit *e) {
    const KAst *ast = e->ast;
    Includes s = { .n = 0 };
    size_t rank[MAX_INCLUDES];
    for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->kind != K_ISLAND_TYPE || !k_em_handles_use(ast, is)) continue;
        bool field = false;
        for (size_t k = 0; k < ast->nodes.len && !field; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            field = n->kind == K_AST_TYPE && is->anchor >= n->first && is->anchor < n->end && !k_em_skip(e, n);
        }
        if (!field) continue;
        Detail d;
        detail_of(ast, is, &d);
        size_t end = span_end(ast, is->anchor, &d);
        if (end < ast->tokens.len && k_em_text_is(k_em_tok(ast, end), "*")) continue;
        char path[NAME_CAP];
        size_t pl = k_em_header_of(ast, d.sym, d.sym_len, ".type.h", path, sizeof path);
        if (!pl) { e->ok = false; return; }
        size_t before = s.n;
        if (!k_em_add_include(&s, path, pl)) { e->ok = false; return; }
        if (s.n > before) rank[s.n - 1] = k_em_import_rank(ast, d.sym, d.sym_len);
    }
    for (size_t i = 1; i < s.n; i++)
        for (size_t j = i; j > 0 && rank[j - 1] > rank[j]; j--) {
            char t[NAME_CAP]; size_t tl = s.len[j], tr = rank[j];
            memcpy(t, s.path[j], tl); memcpy(s.path[j], s.path[j - 1], s.len[j - 1]); s.len[j] = s.len[j - 1]; rank[j] = rank[j - 1];
            memcpy(s.path[j - 1], t, tl); s.len[j - 1] = tl; rank[j - 1] = tr;
        }
    for (size_t k = 0; k < s.n; k++) k_em_raw_include(e, s.path[k], s.len[k]);
}

/* .type.h of an instance that is a unit of its own: the .type.h of what the typedefs
 * and the modifier name, in order of first use (backend §4.3.2, rule 2). A type of
 * another module is an island (`corot`); a binder is the instance's argument, whose
 * header is the program's (`ag2.type.h` for `ag2_Ag`). The unit's own types, and the
 * C types (`i32`), need none. */
void k_em_instance_type_includes(Emit *e) {
    const KAst *ast = e->ast;
    Includes s = { .n = 0 };
    for (size_t i = 0; i < ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if ((n->kind != K_AST_TYPE && n->kind != K_AST_MODIFIER) || !k_em_generic_out(ast, n)) continue;
        for (size_t j = n->first; j < n->end && e->ok; j++) {
            char path[NAME_CAP];
            size_t pl = 0;
            keel_slice_char arg;
            KToken t = k_em_tok(ast, j);
            bool member = j > 0 && (k_em_text_is(k_em_tok(ast, j - 1), ".") || k_em_text_is(k_em_tok(ast, j - 1), "->"));
            if (!member && k_token_is_ident(t) && k_em_binder_arg(e, t, &arg)) {
                if (e->inst->user && !k_em_unqualified(arg).len) continue;
                if (e->inst->user) pl = k_em_header_of(e->inst->user, arg.ptr, arg.len, ".type.h", path, sizeof path);
            } else if (!(k_em_declared_at(e, j) && k_em_generic_out(ast, k_em_declared_at(e, j)))) {
                for (size_t q = 0; q < ast->islands.len && !pl; q++) {
                    const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, q);
                    Detail d;
                    if (is->kind != K_ISLAND_TYPE || is->anchor != j || !k_em_handles_use(ast, is) || !detail_of(ast, is, &d)) continue;
                    pl = k_em_header_of(ast, d.sym, d.sym_len, ".type.h", path, sizeof path);
                }
            }
            if (pl && pl != SIZE_MAX && !k_em_add_include(&s, path, pl)) { e->ok = false; return; }
        }
    }
    for (size_t k = 0; k < s.n; k++) k_em_raw_include(e, s.path[k], s.len[k]);
}
