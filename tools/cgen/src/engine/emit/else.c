/* engine/emit/else.c — the `else` of a result (emit_internal.h; codegen design E5:
 * one file per construction; spec §4.10, backend §5.12). The statement goes out as
 * written, and the test follows it on the same line:
 *
 *   exit:     `T x = e else tail;`  ->  `T x = e; if (F(x)) tail;`
 *   default:  `T x = e else v;`    ->  `T x = e; if (F(x)) W(&x, v);`
 *
 * where F is the `failed` symbol of the island's detail, and W its `win` symbol
 * (marked `&1`: the first argument takes `&`). The `else` token is the island's
 * anchor. The edits are: the bytes from the end of the token before `else` up to
 * the tail (`else`, its blanks) become `; if (F(x)) ` (and `W(&x, ` for default);
 * the `;` that ends a default value gets the `)` that closes W.
 *
 * Refused (k_em_handles_else answers false, and the module is refused with
 * implementation-limit): a detail that does not read as the form above, a win
 * without `&1`, an `else` with no token after it, and a default with no `;` that
 * ends it. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define NO_TOKEN ((size_t)-1)
#define ARROW "\xE2\x86\x92"

/* the parts of an `else` detail: `<target> (<type>) exit|default → <F> [<W> [&1]]` */
typedef struct {
    const char *tgt; size_t tgt_len;    /* the written target: `v`, `s` */
    bool exit;                          /* `exit`: the tail is a statement */
    const char *fail; size_t fail_len;  /* the `failed` symbol */
    const char *win; size_t win_len;    /* the `win` symbol (default only) */
} ElseParts;

static bool read_parts(const KAst *ast, const KIsland *is, ElseParts *x) {
    const char *t = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, arrow = len;
    for (size_t k = 0; k + 3 <= len; k++)
        if (memcmp(t + k, ARROW, 3) == 0) { arrow = k; break; }
    if (arrow == len) return false;
    size_t l = arrow;
    while (l > 0 && t[l - 1] == ' ') l--;
    /* the mode is the last word before the arrow */
    size_t sp = l;
    while (sp > 0 && t[sp - 1] != ' ') sp--;
    size_t mode_len = l - sp;
    if (mode_len == 4 && memcmp(t + sp, "exit", 4) == 0) x->exit = true;
    else if (mode_len == 7 && memcmp(t + sp, "default", 7) == 0) x->exit = false;
    else return false;
    /* the target is the first word */
    size_t tg = 0;
    while (tg < l && t[tg] != ' ') tg++;
    x->tgt = t; x->tgt_len = tg;
    if (tg == 0 || sp <= tg) return false;
    /* the symbols after the arrow */
    size_t s = arrow + 3;
    while (s < len && t[s] == ' ') s++;
    size_t e = s;
    while (e < len && t[e] != ' ') e++;
    if (e == s) return false;
    x->fail = t + s; x->fail_len = e - s;
    x->win = NULL; x->win_len = 0;
    while (e < len && t[e] == ' ') e++;
    if (e < len) {
        size_t w = e;
        while (e < len && t[e] != ' ') e++;
        x->win = t + w; x->win_len = e - w;
        while (e < len && t[e] == ' ') e++;
    }
    /* what follows the win symbol: nothing, or `&1` (the first argument, the target, by address) */
    size_t marks_len = len - e;
    bool amp = marks_len == 2 && memcmp(t + e, "&1", 2) == 0;
    if (x->exit) return x->win == NULL && marks_len == 0;
    return x->win != NULL && amp;
}

/* the `;` that ends the default value after `else` (token at + 1), at depth 0, or NO_TOKEN */
static size_t value_end(const KAst *ast, size_t at) {
    size_t depth = 0;
    for (size_t i = at + 1; i < ast->tokens.len; i++) {
        KToken t = k_em_tok(ast, i);
        if (k_em_text_is(t, "(") || k_em_text_is(t, "[") || k_em_text_is(t, "{")) depth++;
        else if (k_em_text_is(t, ")") || k_em_text_is(t, "]") || k_em_text_is(t, "}")) {
            if (depth == 0) return NO_TOKEN;
            depth--;
        } else if (depth == 0 && k_em_text_is(t, ";")) return i;
    }
    return NO_TOKEN;
}

bool k_em_handles_else(const KAst *ast, const KIsland *is) {
    if (is->kind != K_ISLAND_ELSE || is->anchor + 1 >= ast->tokens.len) return false;
    if (!k_em_text_is(k_em_tok(ast, is->anchor), "else")) return false;
    ElseParts x;
    if (!read_parts(ast, is, &x)) return false;
    return x.exit || value_end(ast, is->anchor) != NO_TOKEN;
}

/* appends x when its position is in the line [a, b) */
static void push_in_line(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, Edit x) {
    if (x.at >= a && x.at < b) k_em_push_edit(e, ed, cnt, x);
}

static size_t put(char *w, size_t q, const char *s, size_t n) {
    memcpy(w + q, s, n);
    return q + n;
}

void k_em_edits_else(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (!k_em_handles_else(ast, is)) continue;
        size_t at = is->anchor;
        if (at < fn->first || at >= fn->end) continue;
        /* only the `else` of this line: the scratch holds one line's text */
        if (k_em_tok_off(ast, at) < a || k_em_tok_off(ast, at) >= b) continue;
        ElseParts x;
        if (!read_parts(ast, is, &x)) continue;

        /* the header: from the end of the token before `else` up to the tail */
        size_t from = k_em_tok_end(ast, at - 1), to = k_em_tok_off(ast, at + 1);
        size_t need = x.fail_len + x.tgt_len + (x.exit ? 0 : x.win_len + x.tgt_len) + 32;
        char *w = k_em_scratch(e, need);
        if (!w) return;
        size_t q = 0;
        q = put(w, q, "; if (", 6);
        q = put(w, q, x.fail, x.fail_len);
        q = put(w, q, "(", 1);
        q = put(w, q, x.tgt, x.tgt_len);
        q = put(w, q, ")) ", 3);
        if (!x.exit) {
            q = put(w, q, x.win, x.win_len);
            q = put(w, q, "(&", 2);
            q = put(w, q, x.tgt, x.tgt_len);
            q = put(w, q, ", ", 2);
        }
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = from, .skip = to - from,
            .text = w, .text_len = q });

        /* the default value ends with the `;` of the statement: W's `)` goes before it */
        if (!x.exit) {
            size_t semi = value_end(ast, at);
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, semi), .skip = 0,
                .text = ")", .text_len = 1 });
        }
    }
}
