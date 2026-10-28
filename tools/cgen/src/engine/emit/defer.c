/* engine/emit/defer.c — `defer` of a function body (emit_internal.h; codegen
 * design E5, backend §5.5, degree D1). Lowered here: a `defer` whose scope ends
 * by the natural end of its block. The registration goes out (an empty line
 * for `defer` and `defer [later]`; a struct copy for `defer [now ...]`), and the
 * body of every `defer` registered in a block is written before the `}` that
 * closes the block, in reverse order of registration, on the line of that `}`.
 *
 * Refused (k_em_handles_defer answers false, and the module is refused with
 * implementation-limit): a function with `return`, `break`, `continue` or `goto`
 * (degrees D2 and D3), a `defer` statement that spans lines, has a comment, a
 * body with braces or an index, or a `}` that is not the first token of its line. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define NO_TOKEN ((size_t)-1)
#define MAX_CAP 8           /* the entries of one `[now ...]` list */

/* one entry of `[now T name]`: the type's tokens [type_first, type_end), and the name token */
typedef struct { size_t type_first, type_end, name; } Cap;

/* a `defer` statement, read from the tokens */
typedef struct {
    size_t body;            /* the body's first token */
    size_t semi;            /* the `;` that ends the statement */
    bool now;               /* a `[now ...]` capture */
    Cap cap[MAX_CAP];
    size_t ncap;
} Defer;

static bool word(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

/* an identifier or a keyword-like word (a type name, a binder), not a number */
static bool word_like(const KAst *ast, size_t i) {
    if (i >= ast->tokens.len) return false;
    KToken t = k_em_tok(ast, i);
    char c = t.len ? t.ptr[0] : 0;
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool same_text(KToken a, KToken b) {
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
}

/* the `]` that closes the `[` at `open`, or NO_TOKEN */
static size_t close_square(const KAst *ast, size_t open) {
    size_t depth = 0;
    for (size_t i = open; i < ast->tokens.len; i++) {
        if (word(ast, i, "[")) depth++;
        else if (word(ast, i, "]") && --depth == 0) return i;
    }
    return NO_TOKEN;
}

/* The function whose body holds token tok, or NULL. */
static const KAstNode *fn_of(const KAst *ast, size_t tok) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind == K_AST_FUNCTION && n->body_first != NO_TOKEN && n->body_end != NO_TOKEN &&
            tok >= n->body_first && tok < n->body_end) return n;
    }
    return NULL;
}

/* whether the body of fn has a jump that leaves a scope (D2 and D3: refused for now) */
static bool jumps(const KAst *ast, const KAstNode *fn) {
    for (size_t i = fn->body_first; i < fn->body_end; i++)
        if (word(ast, i, "return") || word(ast, i, "break") || word(ast, i, "continue") || word(ast, i, "goto"))
            return true;
    return false;
}

/* The `{` that opens the block holding token `from`, searched back from from-1
 * down to `lo` (the function's own `{` is body_first - 1); NO_TOKEN if none. */
static size_t block_open(const KAst *ast, size_t from, size_t lo) {
    size_t depth = 0;
    for (size_t j = from; j-- > lo; ) {
        if (word(ast, j, "}")) depth++;
        else if (word(ast, j, "{")) {
            if (depth == 0) return j;
            depth--;
        }
    }
    return NO_TOKEN;
}

/* The `}` that closes the block opened at `open`, or NO_TOKEN. */
static size_t block_close(const KAst *ast, size_t open) {
    size_t depth = 0;
    for (size_t j = open + 1; j < ast->tokens.len; j++) {
        if (word(ast, j, "{")) depth++;
        else if (word(ast, j, "}")) {
            if (depth == 0) return j;
            depth--;
        }
    }
    return NO_TOKEN;
}

/* whether token `tok` is the first token of its line (only blanks before it) */
static bool first_on_line(const KAst *ast, size_t tok) {
    const char *s = ast->source.ptr;
    size_t p = k_em_tok_off(ast, tok);
    while (p > 0 && (s[p - 1] == ' ' || s[p - 1] == '\t')) p--;
    return p == 0 || s[p - 1] == '\n';
}

/* Reads the `defer` at token kw: `defer [later] body;`, `defer body;` or
 * `defer [now T1 a, T2 b] body;`, with a body that is one simple statement on
 * the line of the `defer`. False for every other form. */
static bool analyze(const KAst *ast, size_t kw, Defer *d) {
    if (!word(ast, kw, "defer")) return false;
    d->now = false;
    d->ncap = 0;
    size_t body = kw + 1;
    if (word(ast, kw + 1, "[")) {
        size_t close = close_square(ast, kw + 1);
        if (close == NO_TOKEN) return false;
        if (word(ast, kw + 2, "later")) {
            if (close != kw + 3) return false;
        } else if (word(ast, kw + 2, "now")) {
            d->now = true;
            size_t st = kw + 3, depth = 0;
            for (size_t j = kw + 3; j <= close; j++) {
                if (j < close && (word(ast, j, "(") || word(ast, j, "[") || word(ast, j, "{"))) depth++;
                else if (j < close && (word(ast, j, ")") || word(ast, j, "]") || word(ast, j, "}"))) {
                    if (depth == 0) return false;
                    depth--;
                } else if (j == close || (depth == 0 && word(ast, j, ","))) {
                    /* an entry is a type (one or more tokens) and a name, the last token */
                    if (j < st + 2 || !word_like(ast, j - 1)) return false;
                    if (d->ncap == MAX_CAP) return false;
                    d->cap[d->ncap++] = (Cap){ st, j - 1, j - 1 };
                    st = j + 1;
                }
            }
        } else return false;
        body = close + 1;
    }
    size_t semi = NO_TOKEN;
    for (size_t j = body; j < ast->tokens.len; j++) {
        if (word(ast, j, ";")) { semi = j; break; }
        if (word(ast, j, "{") || word(ast, j, "}") || word(ast, j, "[")) return false;
    }
    if (semi == NO_TOKEN || semi <= body) return false;
    /* one line, no comment (the copy would carry it into a line of C), no index */
    const char *s = ast->source.ptr;
    size_t from = k_em_tok_off(ast, kw), to = k_em_tok_end(ast, semi);
    for (size_t p = from; p < to; p++) {
        if (s[p] == '\n') return false;
        if (s[p] == '/' && p + 1 < to && (s[p + 1] == '/' || s[p + 1] == '*')) return false;
    }
    d->body = body;
    d->semi = semi;
    return true;
}

/* the number of blanks (spaces and tabs) right before token kw, on its line */
static size_t indent_before(const KAst *ast, size_t kw) {
    const char *s = ast->source.ptr;
    size_t p = k_em_tok_off(ast, kw), n = 0;
    while (n < p && (s[p - n - 1] == ' ' || s[p - n - 1] == '\t')) n++;
    return n;
}

/* the captures `keel__c<N>` of the function: the [now] defers before kw, from the
 * start of the body (backend §2.3, rule 2: a counter per function) */
static size_t capture_ordinal(const KAst *ast, const KAstNode *fn, size_t kw) {
    size_t k = 0;
    for (size_t i = fn->body_first; i < kw; i++)
        if (word(ast, i, "defer") && word(ast, i + 1, "[") && word(ast, i + 2, "now")) k++;
    return k;
}

static void put_uint(Emit *e, size_t v) {
    char dig[24];
    size_t n = 0;
    do { dig[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) k_em_put_bytes(e, &dig[--n], 1);
}

static void put_name(Emit *e, size_t tok) {
    KToken t = k_em_tok(e->ast, tok);
    k_em_put_bytes(e, t.ptr, t.len);
}

/* The body of a defer into e->buf: the tokens of the statement, with the gaps
 * between them as written, every name renamed as in the C text (k_em_put_range);
 * and under [now], a captured name becomes keel__c<n>.<name> (not after `.` or `->`). */
static void put_body(Emit *e, const Defer *d, size_t n) {
    const KAst *ast = e->ast;
    for (size_t i = d->body; i < d->semi && e->ok; i++) {
        if (i > d->body) {
            size_t g0 = k_em_tok_end(ast, i - 1), g1 = k_em_tok_off(ast, i);
            k_em_put_bytes(e, ast->source.ptr + g0, g1 - g0);
        }
        bool member = i > d->body && (word(ast, i - 1, ".") || word(ast, i - 1, "->"));
        bool captured = false;
        if (d->now && !member)
            for (size_t c = 0; c < d->ncap && !captured; c++)
                captured = same_text(k_em_tok(ast, i), k_em_tok(ast, d->cap[c].name));
        if (captured) {
            k_em_put_bytes(e, "keel__c", 7);
            put_uint(e, n);
            k_em_put_bytes(e, ".", 1);
            put_name(e, i);
        } else {
            k_em_put_range(e, i, i + 1);
        }
    }
}

/* Moves the text in e->buf into a scratch copy that the edit points to, and
 * empties the line builder. NULL when the scratch is full (e->ok is false). */
static char *take_buf(Emit *e, size_t *len) {
    char *s = k_em_scratch(e, e->n);
    if (!s) { e->n = 0; return NULL; }
    memcpy(s, e->buf, e->n);
    *len = e->n;
    e->n = 0;
    return s;
}

/* Removes the edits whose position is in [lo, hi): the statement is rebuilt
 * from its tokens, so the renames the emitter pushed for it are replaced. */
static void drop_edits(Edit *ed, size_t *cnt, size_t lo, size_t hi) {
    size_t j = 0;
    for (size_t k = 0; k < *cnt; k++)
        if (ed[k].at < lo || ed[k].at >= hi) ed[j++] = ed[k];
    *cnt = j;
}

/* appends x when its position is in the line [a, b) */
static void push_in_line(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, Edit x) {
    if (x.at >= a && x.at < b) k_em_push_edit(e, ed, cnt, x);
}

void k_em_edits_defer(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    if (fn->body_first == NO_TOKEN || fn->body_end == NO_TOKEN) return;

    /* the registrations that start on this line */
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        size_t kw = is->anchor;
        if (is->kind != K_ISLAND_DEFER || kw < fn->body_first || kw >= fn->body_end) continue;
        size_t at = k_em_tok_off(ast, kw);
        if (at < a || at >= b) continue;
        Defer d;
        if (!analyze(ast, kw, &d)) { e->ok = false; return; }
        size_t semi_end = k_em_tok_end(ast, d.semi);
        size_t ind = indent_before(ast, kw);
        if (!d.now) {
            /* the statement and its indentation leave the line: the line is empty */
            drop_edits(ed, cnt, at - ind, semi_end);
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_DROP, .at = at - ind, .skip = semi_end - (at - ind) });
        } else {
            /* the registration: struct { T1 a; T2 b; } keel__c<N> = { a, b }; */
            drop_edits(ed, cnt, at, semi_end);
            e->n = 0;
            k_em_put_bytes(e, "struct { ", 9);
            for (size_t c = 0; c < d.ncap; c++) {
                k_em_put_range(e, d.cap[c].type_first, d.cap[c].type_end);
                k_em_put_bytes(e, " ", 1);
                put_name(e, d.cap[c].name);
                k_em_put_bytes(e, "; ", 2);
            }
            k_em_put_bytes(e, "} keel__c", 9);
            put_uint(e, capture_ordinal(ast, fn, kw));
            k_em_put_bytes(e, " = { ", 5);
            for (size_t c = 0; c < d.ncap; c++) {
                if (c) k_em_put_bytes(e, ", ", 2);
                put_name(e, d.cap[c].name);
            }
            k_em_put_bytes(e, " };", 3);
            size_t len;
            char *text = take_buf(e, &len);
            if (!e->ok || !text) { e->ok = false; return; }
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = semi_end - at,
                .text = text, .text_len = len });
        }
    }

    /* the cleanups: at each `}` on this line, the bodies of the defers that the
     * block holds, in reverse order of registration, then a space and the `}` */
    for (size_t c = fn->body_first; c <= fn->body_end && c < ast->tokens.len && e->ok; c++) {
        if (!word(ast, c, "}")) continue;
        size_t at = k_em_tok_off(ast, c);
        if (at < a || at >= b) continue;
        size_t open = block_open(ast, c, fn->body_first - 1);
        if (open == NO_TOKEN) { e->ok = false; return; }
        e->n = 0;
        bool any = false;
        for (size_t j = c; j-- > open + 1; ) {
            if (!word(ast, j, "defer")) continue;
            Defer d;
            if (!analyze(ast, j, &d)) { e->ok = false; return; }
            if (block_open(ast, j, fn->body_first - 1) != open) continue;
            if (!any) {
                /* the indentation of the defer that is written first (the last one registered) */
                size_t ind = indent_before(ast, j);
                k_em_put_bytes(e, ast->source.ptr + k_em_tok_off(ast, j) - ind, ind);
            }
            put_body(e, &d, capture_ordinal(ast, fn, j));
            k_em_put_bytes(e, "; ", 2);
            any = true;
        }
        if (!any) continue;
        /* the blanks before the `}` are replaced: the line starts with the defer's indentation */
        size_t bl = indent_before(ast, c);
        size_t len;
        char *text = take_buf(e, &len);
        if (!e->ok || !text) { e->ok = false; return; }
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at - bl, .skip = bl,
            .text = text, .text_len = len });
    }
}

bool k_em_handles_defer(const KAst *ast, const KIsland *is) {
    if (is->kind != K_ISLAND_DEFER) return false;
    const KAstNode *fn = fn_of(ast, is->anchor);
    if (!fn || jumps(ast, fn)) return false;
    Defer d;
    if (!analyze(ast, is->anchor, &d)) return false;
    size_t open = block_open(ast, is->anchor, fn->body_first - 1);
    if (open == NO_TOKEN) return false;
    size_t close = block_close(ast, open);
    if (close == NO_TOKEN || close > fn->body_end) return false;
    /* the cleanup goes before the `}`, on its line: the `}` must start the line */
    return first_on_line(ast, close);
}
