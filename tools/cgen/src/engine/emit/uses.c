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

/* The marks are `&<k>` (argument k by address) and `type:<k>` (argument k is
 * a type, erased into its size and alignment: backend §5.16); out: a bit per
 * argument (1-based) for each. */
static bool marks_of(const Detail *d, unsigned *amp, unsigned *type) {
    *amp = 0;
    *type = 0;
    size_t p = 0;
    while (p < d->marks_len) {
        if (d->marks[p] == ' ') { p++; continue; }
        unsigned *set = amp;
        if (d->marks[p] == '&') p++;
        else if (p + 5 <= d->marks_len && memcmp(d->marks + p, "type:", 5) == 0) { p += 5; set = type; }
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

bool k_em_foreign_island(const KAst *ast, const KIsland *is) {
    if (is->kind != K_ISLAND_TYPE && is->kind != K_ISLAND_NAME && is->kind != K_ISLAND_CALL) return false;
    Detail d;
    if (!detail_of(ast, is, &d) || declared_here(ast, &d)) return false;
    return true;
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
    return s && s->kind == K_SYM_FUNCTION && s->origin ? s->origin->ast : NULL;
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

/* whether a foreign island anchors in the tokens [a, b) */
static bool foreign_in(const KAst *ast, size_t a, size_t b) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->anchor >= a && is->anchor < b && k_em_foreign_island(ast, is)) return true;
    }
    return false;
}

bool k_em_handles_use(const KAst *ast, const KIsland *is) {
    Detail d;
    if (!k_em_foreign_island(ast, is) || !detail_of(ast, is, &d)) return false;
    if (d.sym_len == 4 && memcmp(d.sym, "core", 4) == 0) return false;   /* lowered by the core: not a rename */
    unsigned amp, type;
    if (!marks_of(&d, &amp, &type)) return false;
    size_t end = span_end(ast, is->anchor, &d);
    if (end == NO_TOKEN) return false;
    if (is->kind != K_ISLAND_CALL) return amp == 0 && type == 0;
    size_t first[17], last[17], argc;
    if (!call_args(ast, end, first, last, &argc)) return false;
    for (unsigned k = 1; k <= 16; k++) {
        if (((amp | type) & (1u << k)) && k > argc) return false;
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

/* removes the edits whose position is in [lo, hi) (a span copied again) */
static void drop_edits(Edit *ed, size_t *cnt, size_t lo, size_t hi) {
    size_t j = 0;
    for (size_t k = 0; k < *cnt; k++)
        if (ed[k].at < lo || ed[k].at >= hi) ed[j++] = ed[k];
    *cnt = j;
}

void k_em_edits_uses(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    const char *align = e->opt.c23 ? "alignof(" : "_Alignof(";
    for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->anchor < n->first || is->anchor >= n->end) continue;
        if (k_em_handles_implicit_init(ast, is)) {
            size_t at = k_em_tok_end(ast, is->anchor);
            if (at > a && at <= b)
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0, .text = " = {0}", .text_len = 6 });
            continue;
        }
        if (!k_em_handles_use(ast, is)) {
            /* a call of the module's own verb: the name is renamed by emit.c, the `&` are here */
            Detail o;
            unsigned amp, type;
            size_t f[17], l[17], n_args, end;
            if (is->kind != K_ISLAND_CALL || k_em_foreign_island(ast, is) || !detail_of(ast, is, &o) ||
                !marks_of(&o, &amp, &type) || amp == 0 || (end = span_end(ast, is->anchor, &o)) == NO_TOKEN ||
                !call_args(ast, end, f, l, &n_args)) continue;
            for (size_t k = 1; k <= n_args; k++) {
                size_t p = k_em_tok_off(ast, f[k]);
                if ((amp & (1u << k)) && p >= a && p < b)
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = 0, .text = "&", .text_len = 1 });
            }
            continue;
        }
        Detail d;
        detail_of(ast, is, &d);
        size_t end = span_end(ast, is->anchor, &d);
        size_t at = k_em_tok_off(ast, is->anchor);
        unsigned amp = 0, type = 0;
        size_t first[17], last[17], argc = 0;
        if (is->kind == K_ISLAND_CALL) {
            marks_of(&d, &amp, &type);
            call_args(ast, end, first, last, &argc);
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
            if (amp & (1u << k))
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = 0, .text = "&", .text_len = 1 });
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
            if (k_em_text_is(t, "alignof"))
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = t.len, .text = "_Alignof", .text_len = 8 });
            else if (k_em_text_is(t, "alignas"))
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = t.len, .text = "_Alignas", .text_len = 8 });
        }
    k_em_sort_edits(ed, 0, *cnt);
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
        if (n->kind != K_AST_FUNCTION || !n->is_public || n->body_first == NO_TOKEN || n->body_first < 1) continue;
        for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
            const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
            if (is->kind != K_ISLAND_TYPE || is->anchor < n->first || is->anchor >= n->body_first - 1) continue;
            if (!k_em_handles_use(ast, is)) continue;
            Detail d;
            detail_of(ast, is, &d);
            size_t end = span_end(ast, is->anchor, &d);
            if (end >= ast->tokens.len || !k_em_text_is(k_em_tok(ast, end), "*")) { e->ok = false; return; }
            /* once: not if an earlier island of a public signature has the same symbol */
            bool seen = false;
            for (size_t j = 0; j < i && !seen; j++) {
                const KIsland *o = keel_buffer_KIsland_ptr(&ast->islands, j);
                Detail od;
                if (o->kind != K_ISLAND_TYPE || !k_em_handles_use(ast, o) || !detail_of(ast, o, &od)) continue;
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

/* ---- the includes of the .c (rule 4) ---- */

static bool prefix_of(const char *sym, size_t len, const char *p, size_t plen) {
    return len == plen ? memcmp(sym, p, plen) == 0 : len > plen && memcmp(sym, p, plen) == 0 && sym[plen] == '_';
}

/* the instance of the closure the symbol belongs to (the longest), or -1 */
static int instance_of(const KAst *ast, const char *sym, size_t len) {
    int best = -1;
    size_t best_len = 0;
    for (size_t c = 0; c < ast->closure.len; c++) {
        const KClosure *k = keel_buffer_KClosure_ptr(&ast->closure, c);
        if (k->kind != K_CLOSURE_INSTANCE) continue;
        if (prefix_of(sym, len, k->symbol, k->symbol_len) && k->symbol_len > best_len) { best = (int)c; best_len = k->symbol_len; }
    }
    return best;
}

/* the dotted name of an import node, into buf */
static keel_slice_char import_name(const KAst *ast, const KAstNode *n, char *buf, size_t cap) {
    size_t len = 0;
    for (size_t j = n->name_first; j < n->name_end; j++) {
        KToken t = k_em_tok(ast, j);
        if (len + t.len > cap) return (keel_slice_char){ 0 };
        memcpy(buf + len, t.ptr, t.len);
        len += t.len;
    }
    return (keel_slice_char){ len, buf };
}

static void raw_include(Emit *e, const char *path, size_t len) {
    k_emit_raw(&e->w, (keel_slice_char){ 10, (char *)"#include \"" });
    k_emit_raw(&e->w, (keel_slice_char){ len, (char *)path });
    k_emit_raw(&e->w, (keel_slice_char){ 2, (char *)"\"\n" });
}

void k_em_use_includes(Emit *e) {
    const KAst *ast = e->ast;
    /* the modules, in the order of the imports */
    for (size_t k = 0; k < ast->nodes.len && e->ok; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_IMPORT) continue;
        char name_buf[NAME_CAP], sym_buf[NAME_CAP];
        keel_slice_char name = import_name(ast, n, name_buf, sizeof name_buf);
        if (!name.len || k_em_is_keel(name)) continue;
        size_t ms = k_mangle_module(name, (keel_slice_char){ sizeof sym_buf, sym_buf });
        if (ms > sizeof sym_buf) { e->ok = false; return; }
        bool used = false;
        for (size_t i = 0; i < ast->islands.len && !used; i++) {
            const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
            Detail d;
            if (!k_em_handles_use(ast, is) || !detail_of(ast, is, &d)) continue;
            used = instance_of(ast, d.sym, d.sym_len) < 0 && prefix_of(d.sym, d.sym_len, sym_buf, ms) && d.sym_len > ms;
        }
        if (!used) continue;
        char path[NAME_CAP];
        size_t pl = k_mangle_path(name, ".h", (keel_slice_char){ sizeof path, path });
        if (pl > sizeof path) { e->ok = false; return; }
        raw_include(e, path, pl);
    }
    /* the instances, in the order of the closure */
    for (size_t c = 0; c < ast->closure.len && e->ok; c++) {
        const KClosure *k = keel_buffer_KClosure_ptr(&ast->closure, c);
        if (k->kind != K_CLOSURE_INSTANCE || !k->module) continue;
        bool used = false;
        for (size_t i = 0; i < ast->islands.len && !used; i++) {
            const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
            Detail d;
            if (!k_em_handles_use(ast, is) || !detail_of(ast, is, &d)) continue;
            used = instance_of(ast, d.sym, d.sym_len) == (int)c;
        }
        if (!used) continue;
        char path[NAME_CAP];
        size_t pl = k_mangle_instance_path(k->module->name, (keel_slice_char){ k->symbol_len, (char *)k->symbol }, ".h",
                                           (keel_slice_char){ sizeof path, path });
        if (pl > sizeof path) { e->ok = false; return; }
        raw_include(e, path, pl);
    }
}
