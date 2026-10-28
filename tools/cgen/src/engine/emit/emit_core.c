/* engine/emit/emit_core.c — the helpers that emit.c and the construction files
 * share: token offsets, line arithmetic, the line builder, the edit list
 * (emit_internal.h). No I/O, no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"

static size_t off_of(const KAst *ast, const char *p) { return (size_t)(p - ast->source.ptr); }

KToken k_em_tok(const KAst *ast, size_t i) {
    return keel_buffer_KLexeme_ptr(&ast->tokens, i)->token;
}
size_t k_em_tok_off(const KAst *ast, size_t i) { return off_of(ast, k_em_tok(ast, i).ptr); }
size_t k_em_tok_end(const KAst *ast, size_t i) { return k_em_tok_off(ast, i) + k_em_tok(ast, i).len; }
size_t k_em_first_off(const Emit *e, const KAstNode *n) { return k_em_tok_off(e->ast, n->first); }
size_t k_em_last_end(const Emit *e, const KAstNode *n) { return k_em_tok_end(e->ast, n->end - 1); }

bool k_em_text_is(KToken t, const char *s) {
    size_t n = strlen(s);
    return t.len == n && memcmp(t.ptr, s, n) == 0;
}
bool k_em_is_visibility(KToken t) { return k_em_text_is(t, "pub") || k_em_text_is(t, "priv"); }
bool k_em_is_keel(keel_slice_char m) { return m.len == 4 && memcmp(m.ptr, "keel", 4) == 0; }

size_t k_em_line_start(const Emit *e, size_t off) {
    const char *s = e->ast->source.ptr;
    while (off > 0 && s[off - 1] != '\n') off--;
    return off;
}
size_t k_em_line_end(const Emit *e, size_t off) {
    const char *s = e->ast->source.ptr;
    size_t len = e->ast->source.len;
    while (off < len && s[off] != '\n') off++;
    return off < len ? off + 1 : len;
}
/* the scan starts from the last answer, so calls in source order cost one pass
 * over the file */
size_t k_em_line_at(Emit *e, size_t off) {
    const char *s = e->ast->source.ptr;
    if (off < e->cache_off) { e->cache_off = 0; e->cache_line = 1; }
    for (size_t i = e->cache_off; i < off; i++)
        if (s[i] == '\n') e->cache_line++;
    e->cache_off = off;
    return e->cache_line;
}

/* ---- the line builder ---- */

void k_em_put_bytes(Emit *e, const char *p, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (e->n >= LINE_CAP) { e->ok = false; return; }
        e->buf[e->n++] = p[i];
    }
}

/* the mangled name of a declaration, written at the end of buf */
void k_em_put_mangled(Emit *e, size_t name_tok, bool is_type) {
    size_t room = LINE_CAP - e->n;
    size_t need = k_mangle_decl(e->module, k_em_tok(e->ast, name_tok), is_type,
                                (keel_slice_char){ room, e->buf + e->n });
    if (need > room) { e->ok = false; return; }
    e->n += need;
}

/* Adds one edit to the list of a line; a list past MAX_EDITS makes the call
 * UNSUPPORTED. */
void k_em_push_edit(Emit *e, Edit *ed, size_t *cnt, Edit x) {
    if (*cnt >= MAX_EDITS) { e->ok = false; return; }
    ed[(*cnt)++] = x;
}

bool k_em_declares_value(KAstKind k) {
    return k == K_AST_FUNCTION || k == K_AST_VARIABLE || k == K_AST_CONSTEXPR;
}

/* The module-level declaration that token `i` names, or NULL. A name after
 * `.` or `->` is a member, not the declaration. Scope is not tracked: a local
 * or parameter spelled like a module declaration is renamed too (a limit of
 * the slice, M5 brings the symbol table). */
const KAstNode *k_em_declared_at(const Emit *e, size_t i) {
    const KAst *ast = e->ast;
    if (i > 0 && (k_em_text_is(k_em_tok(ast, i - 1), ".") || k_em_text_is(k_em_tok(ast, i - 1), "->"))) return NULL;
    KToken t = k_em_tok(ast, i);
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *d = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (d->kind != K_AST_TYPE && d->kind != K_AST_TAGS && !k_em_declares_value(d->kind)) continue;
        if (d->name_first >= d->name_end) continue;
        KToken n = k_em_tok(ast, d->name_first);
        if (n.len == t.len && memcmp(n.ptr, t.ptr, t.len) == 0) return d;
    }
    return NULL;
}

void k_em_sort_edits(Edit *ed, size_t from, size_t cnt) {
    for (size_t i = from; i < cnt; i++)
        /* at the same position an insertion (skip 0) goes before a replacement */
        for (size_t j = i; j > 0 && (ed[j - 1].at > ed[j].at ||
                (ed[j - 1].at == ed[j].at && ed[j - 1].skip != 0 && ed[j].skip == 0)); j--) {
            Edit t = ed[j]; ed[j] = ed[j - 1]; ed[j - 1] = t;
        }
}

char *k_em_scratch(Emit *e, size_t n) {
    if (e->scratch_n + n > sizeof e->scratch) { e->ok = false; return NULL; }
    char *p = e->scratch + e->scratch_n;
    e->scratch_n += n;
    return p;
}

void k_em_put_range(Emit *e, size_t tok_a, size_t tok_b) {
    const KAst *ast = e->ast;
    for (size_t i = tok_a; i < tok_b && e->ok; i++) {
        if (i > tok_a) {   /* the gap between the two tokens, as written */
            size_t g0 = k_em_tok_end(ast, i - 1), g1 = k_em_tok_off(ast, i);
            k_em_put_bytes(e, ast->source.ptr + g0, g1 - g0);
        }
        char name[NAME_CAP];
        size_t len;
        const KAstNode *d;
        if (k_em_constblock_rename(e, i, name, sizeof name, &len)) k_em_put_bytes(e, name, len);
        else if ((d = k_em_declared_at(e, i)) != NULL)
            k_em_put_mangled(e, d->name_first, d->kind == K_AST_TYPE || d->kind == K_AST_TAGS);
        else k_em_put_bytes(e, k_em_tok(ast, i).ptr, k_em_tok(ast, i).len);
    }
}

bool k_em_overlaps(const Emit *e, const KAstNode *n, size_t a, size_t b) {
    return k_em_first_off(e, n) < b && k_em_last_end(e, n) > a;
}
