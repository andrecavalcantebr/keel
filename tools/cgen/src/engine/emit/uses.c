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

/* The marks are `&<k>`, each argument once; out: a bit per argument (1-based). */
static bool marks_of(const Detail *d, unsigned *amp) {
    *amp = 0;
    size_t p = 0;
    while (p < d->marks_len) {
        if (d->marks[p] == ' ') { p++; continue; }
        if (d->marks[p] != '&') return false;
        p++;
        unsigned k = 0;
        size_t digits = 0;
        while (p < d->marks_len && d->marks[p] >= '0' && d->marks[p] <= '9') { k = k * 10 + (unsigned)(d->marks[p++] - '0'); digits++; }
        if (!digits || k == 0 || k > 16) return false;
        *amp |= 1u << k;
    }
    return true;
}

/* The first token of each argument of the call whose `(` is `open`; false when
 * the call is not well formed. */
static bool call_args(const KAst *ast, size_t open, size_t first[17], size_t *argc) {
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

bool k_em_handles_use(const KAst *ast, const KIsland *is) {
    Detail d;
    if (!k_em_foreign_island(ast, is) || !detail_of(ast, is, &d)) return false;
    if (d.sym_len == 4 && memcmp(d.sym, "core", 4) == 0) return false;   /* lowered by the core: not a rename */
    unsigned amp;
    if (!marks_of(&d, &amp)) return false;
    size_t end = span_end(ast, is->anchor, &d);
    if (end == NO_TOKEN) return false;
    if (is->kind != K_ISLAND_CALL) return amp == 0;
    size_t first[17], argc;
    if (!call_args(ast, end, first, &argc)) return false;
    for (unsigned k = 1; k <= 16; k++) if ((amp & (1u << k)) && k > argc) return false;
    return true;
}

void k_em_edits_uses(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t before = *cnt;
    for (size_t i = 0; i < ast->islands.len && e->ok; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->anchor < n->first || is->anchor >= n->end) continue;
        if (!k_em_handles_use(ast, is)) continue;
        Detail d;
        detail_of(ast, is, &d);
        size_t end = span_end(ast, is->anchor, &d);
        size_t at = k_em_tok_off(ast, is->anchor);
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok_end(ast, end - 1) - at,
                .text = d.sym, .text_len = d.sym_len });
        if (is->kind != K_ISLAND_CALL) continue;
        unsigned amp;
        size_t first[17], argc;
        marks_of(&d, &amp);
        call_args(ast, end, first, &argc);
        for (size_t k = 1; k <= argc; k++) {
            if (!(amp & (1u << k))) continue;
            size_t p = k_em_tok_off(ast, first[k]);
            if (p >= a && p < b)
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = 0, .text = "&", .text_len = 1 });
        }
    }
    k_em_sort_edits(ed, before, *cnt);
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
