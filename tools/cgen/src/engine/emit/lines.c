/* engine/emit/lines.c — the lines of a module file (emit_internal.h; backend §6,
 * rule 3; §5.5, rule 4): the edits of a source line are applied to its text, and
 * each line is written with its own .k number. No I/O, no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"

/* Comments leave the generated text and their lines stay (backend §6, rule 3):
 * a comment is dropped, and so are the blanks before it when it ended the
 * line. Tokens do not include comments, so they sit in the gaps between
 * tokens, which hold nothing else; that is where they are found. The edits
 * are merged into `ed` in source order. */
void k_em_comment_edits(Emit *e, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    const char *s = ast->source.ptr;
    size_t ntok = ast->tokens.len;
    size_t lo = 0, hi = ntok;
    while (lo < hi) {   /* the first token that ends after a */
        size_t mid = (lo + hi) / 2;
        if (k_em_tok_end(ast, mid) > a) hi = mid; else lo = mid + 1;
    }
    size_t before = *cnt;
    size_t g0 = lo > 0 ? k_em_tok_end(ast, lo - 1) : 0;
    for (size_t i = lo; g0 < b; i++) {
        size_t g1 = i < ntok ? k_em_tok_off(ast, i) : ast->source.len;
        /* the scan starts at the gap's start, not at the line's: a block comment
         * that began on an earlier line still covers the start of this one */
        for (size_t p = g0; p < g1 && p < b; ) {
            size_t ce;
            if (s[p] == '/' && p + 1 < g1 && s[p + 1] == '/') {
                ce = p;
                while (ce < g1 && s[ce] != '\n') ce++;
            } else if (s[p] == '/' && p + 1 < g1 && s[p + 1] == '*') {
                ce = p + 2;
                while (ce + 1 < g1 && !(s[ce] == '*' && s[ce + 1] == '/')) ce++;
                ce = ce + 2 < g1 ? ce + 2 : g1;
            } else { p++; continue; }
            size_t cs0 = p, cs = cs0 < a ? a : cs0, end = ce < b ? ce : b;
            if (end > cs && s[end - 1] == '\n') end--;     /* the break stays */
            size_t rest = end;
            while (rest < b && (s[rest] == ' ' || s[rest] == '\t')) rest++;
            if (cs0 >= a && (rest >= b || s[rest] == '\n'))   /* it began here and ended the line */
                while (cs > g0 && cs > a && (s[cs - 1] == ' ' || s[cs - 1] == '\t')) cs--;
            if (end > cs) k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = cs, .skip = end - cs, .name_tok = 0, .is_type = false });
            p = ce > p ? ce : p + 1;
        }
        if (i >= ntok) break;
        g0 = k_em_tok_end(ast, i);
    }
    k_em_sort_edits(ed, before, *cnt);
}

/* Copies source [a, b) into buf, applying the edits (sorted by position). */
/* EDIT_LINES: the line is cut here (see emit_internal.h for the payload) */
static void split_line(Emit *e, size_t ln, const char *p, size_t len) {
    const char *end = p + len;
    const char *tail = NULL;
    size_t tail_len = 0;
    bool flushed = false;
    while (p < end && e->ok) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *rec_end = nl ? nl : end;
        if (*p == 'H') {
            k_em_put_bytes(e, p + 2, (size_t)(rec_end - (p + 2)));
        } else {
            if (!flushed) {   /* the first half goes out now, unless it is only blanks */
                size_t i = 0;
                while (i < e->n && (e->buf[i] == ' ' || e->buf[i] == '\t')) i++;
                if (i < e->n) {
                    k_em_put_bytes(e, "\n", 1);
                    if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
                }
                e->n = 0;
                flushed = true;
            }
            if (*p == 'L') {
                size_t n = 0;
                const char *q = p + 1;
                while (q < rec_end && *q >= '0' && *q <= '9') n = n * 10 + (size_t)(*q++ - '0');
                e->n = 0;             /* e->buf is free: the first half was written */
                k_em_put_bytes(e, q + 1, (size_t)(rec_end - (q + 1)));
                k_em_put_bytes(e, "\n", 1);
                if (e->ok && !k_emit_at(&e->w, n, (keel_slice_char){ e->n, e->buf })) e->ok = false;
                e->n = 0;
            } else if (*p == 'T') {
                tail = p + 2;
                tail_len = (size_t)(rec_end - (p + 2));
            }
        }
        p = nl ? nl + 1 : end;
    }
    if (tail) k_em_put_bytes(e, tail, tail_len);
}

/* the source offset up to which the last copy_edited consumed the text: past its
 * line end when an edit spans the line break (the header of a `parallel` over
 * two lines), and emit_range then leaves the lines that the edit covers out */
static size_t spanned_end;

/* what the last copy_edited consumed (see spanned_end): the file writers (files.c) read it */
size_t k_em_spanned_end(void) { return spanned_end; }

void k_em_copy_edited(Emit *e, size_t ln, size_t a, size_t b, const Edit *ed, size_t cnt) {
    const char *s = e->ast->source.ptr;
    size_t pos = a;
    for (size_t k = 0; k < cnt && e->ok; k++) {
        k_em_put_bytes(e, s + pos, ed[k].at - pos);
        if (ed[k].kind == EDIT_RENAME) k_em_put_mangled(e, ed[k].name_tok, ed[k].is_type);
        else if (ed[k].kind == EDIT_ASSERT) k_em_put_bytes(e, "_Static_assert", 14);
        else if (ed[k].kind == EDIT_STATIC) k_em_put_bytes(e, "static", 6);
        else if (ed[k].kind == EDIT_TAG) { k_em_put_bytes(e, " ", 1); k_em_put_mangled(e, ed[k].name_tok, true); }
        else if (ed[k].kind == EDIT_TEXT) k_em_put_bytes(e, ed[k].text, ed[k].text_len);
        else if (ed[k].kind == EDIT_TOKS) k_em_put_range(e, ed[k].tok_a, ed[k].tok_b);
        else if (ed[k].kind == EDIT_LINES) split_line(e, ln, ed[k].text, ed[k].text_len);
        else if (ed[k].kind == EDIT_ENUMERATOR) {
            k_em_put_mangled(e, ed[k].name_tok, true);
            k_em_put_bytes(e, "_", 1);
            k_em_put_bytes(e, s + ed[k].at, ed[k].skip);
        } else if (ed[k].kind == EDIT_ENUM_CLOSE) {
            k_em_put_bytes(e, "} ", 2);
            k_em_put_mangled(e, ed[k].name_tok, true);
        }
        pos = ed[k].at + ed[k].skip;
    }
    if (e->ok && pos < b) k_em_put_bytes(e, s + pos, b - pos);
    spanned_end = pos;
    e->scratch_n = 0;   /* the edits' text is spent */
}

/* Writes the source lines [a, b) as the .k line `ln` (one call: a changed
 * line is one text, and the writer owns its #line). */
/* A construction that rewrites a span whole (the `return` of an inline `defer`
 * cleanup, a use of another module) rebuilds its text with the renames already
 * applied (k_em_put_range): an edit another construction made inside that span
 * is dropped, as uses.c does for its own (drop_edits). The edits are sorted. */
void k_em_drop_nested(Edit *ed, size_t *cnt) {
    size_t j = 0, end = 0;
    for (size_t k = 0; k < *cnt; k++) {
        if (j > 0 && ed[k].at < end && (ed[k].at > ed[j - 1].at || ed[k].skip > 0)) continue;   /* inside the previous replacement (a replacement at the same place too) */
        ed[j++] = ed[k];
        if (ed[k].at + ed[k].skip > end) end = ed[k].at + ed[k].skip;
    }
    *cnt = j;
}

void k_em_emit_line(Emit *e, size_t ln, size_t a, size_t b) {
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    spanned_end = 0;
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (k_em_overlaps(e, n, a, b)) k_em_node_edits(e, n, a, b, ed, &cnt);
    }
    k_em_comment_edits(e, a, b, ed, &cnt);
    k_em_drop_nested(ed, &cnt);
    if (!e->ok) return;
    const char *s = e->ast->source.ptr;
    k_em_pre_lines_constblock(e, ln, a, b);
    if (cnt == 0 && b > a && s[b - 1] == '\n') {
        if (!k_emit_at(&e->w, ln, (keel_slice_char){ b - a, (char *)s + a })) e->ok = false;
    } else {
        e->n = 0;
        k_em_copy_edited(e, ln, a, b, ed, cnt);
        if (e->n == 0 || e->buf[e->n - 1] != '\n') k_em_put_bytes(e, "\n", 1);
        if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
    }
    k_em_post_lines_constblock(e, ln, a, b);
}

/* one source line [a, b) with its .k number; a one-line parallel block is written as two
 * parts (its header, and its body from the cut on), so the body gets its own #line */
void k_em_emit_split(Emit *e, size_t ln, size_t a, size_t b) {
    size_t cut = k_em_parallel_cut(e->ast, a, b);
    if (cut) {
        k_em_emit_line(e, ln, a, cut);
        k_em_emit_line(e, ln, cut, b);
    } else {
        k_em_emit_line(e, ln, a, b);
    }
}

/* the whole lines [a, b), each with its own .k number, byte for byte: no comment
 * stripping and no node edit — the content of extern_c (backend §4.3.2, rules 9, 10),
 * where keel does not look inside (linguagem §2.1) */
void k_em_emit_opaque_range(Emit *e, size_t a, size_t b) {
    const char *s = e->ast->source.ptr;
    for (size_t ln = k_em_line_at(e, a); a < b && e->ok; ln++) {
        size_t next = k_em_line_end(e, a);
        if (!k_emit_at(&e->w, ln, (keel_slice_char){ next - a, (char *)s + a })) e->ok = false;
        a = next;
    }
}

/* the whole lines [a, b), each with its own .k number */
void k_em_emit_range(Emit *e, size_t a, size_t b) {
    size_t ln = k_em_line_at(e, a);
    size_t written = 0;   /* the lines before this offset went out with an edit that spans them */
    while (a < b && e->ok) {
        size_t next = k_em_line_end(e, a);
        if (a >= written) {
            k_em_emit_split(e, ln, a, next);
            if (spanned_end > next) written = spanned_end;
        }
        a = next;
        ln++;
    }
}

