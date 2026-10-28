/* engine/emit/defer.c — `defer` of a function body (emit_internal.h; codegen
 * design E5, backend §5.5, degrees D1 and D2). The registration goes out (an
 * empty line for `defer` and `defer [later]`; a struct copy for `defer [now ...]`),
 * and every exit of the function runs the bodies of the defers registered before
 * it, in reverse order of registration (backend §5.5, rules 1 and 4):
 *
 *   - the natural end of a block: its `}` gets one line per defer (D1);
 *   - a `return` that is the only exit of the function (RET_INLINE): the
 *     cleanup goes in the `return`'s own line, between braces (backend §5.5.2);
 *   - two exits or more (RET_LADDER): the `return` stores its value in
 *     keel__rv0 and jumps to a label, and the labels of the defers are a ladder
 *     before the closing `}` of the function (backend §5.5.2).
 *
 *   - a `break` or `continue` anywhere in the function (D3): every exit is
 *     inline (RET_INLINE), and each `break`/`continue` runs the defers of the
 *     blocks it leaves, up to the innermost loop (or switch, for `break`)
 *     (backend §5.5, rule 2; §5.5.2, rules 1, 2 and 5).
 *
 * Each cleanup line is mapped to the line of the `defer` that wrote it, through
 * EDIT_LINES (backend §5.5, rule 4; D61). Refused (k_em_handles_defer answers
 * false, and the module is refused with implementation-limit): `goto`; a
 * `defer` statement that spans lines, has a comment, a body with braces or an
 * index; a `defer` in a nested block of a function that has a `return` but no
 * `break` or `continue` (that would be a ladder per block); a `}` that is not
 * the first token of its line; a `void` function with two exits; a `break` or
 * `continue` with no loop to leave; and a `return` that does not fit its mode
 * (multi-line or with an index, for RET_INLINE). */
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

/* how the exits of a function with defers are lowered */
typedef enum { RET_NONE, RET_INLINE, RET_LADDER } RetMode;

typedef struct {
    RetMode mode;
    bool ok;                /* false: the function is refused */
    bool ends_ret;          /* the body ends in a `return`: the natural end is no exit */
} Plan;

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

/* whether the body of fn has a `goto` (refused for now) */
static bool has_goto(const KAst *ast, const KAstNode *fn) {
    for (size_t i = fn->body_first; i < fn->body_end; i++)
        if (word(ast, i, "goto")) return true;
    return false;
}

/* whether the body of fn has a `break` or a `continue` (D3: every exit is inline) */
static bool has_jump(const KAst *ast, const KAstNode *fn) {
    for (size_t i = fn->body_first; i < fn->body_end; i++)
        if (word(ast, i, "break") || word(ast, i, "continue")) return true;
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

/* What the `{` at open opens: 1 a loop body (`for (...)`, `while (...)`, `do`),
 * 2 a switch body, 0 anything else. */
static int block_kind(const KAst *ast, size_t open) {
    if (open == NO_TOKEN || open == 0) return 0;
    if (word(ast, open - 1, "do")) return 1;
    if (!word(ast, open - 1, ")")) return 0;
    size_t depth = 0;
    for (size_t q = open - 1; q-- > 0; ) {
        if (word(ast, q, ")")) depth++;
        else if (word(ast, q, "(")) {
            if (depth > 0) { depth--; continue; }
            if (q == 0) return 0;
            if (word(ast, q - 1, "for") || word(ast, q - 1, "while")) return 1;
            if (word(ast, q - 1, "switch")) return 2;
            return 0;
        }
    }
    return 0;
}

/* The `{` of the innermost loop (or switch, when want_switch) that holds token p,
 * or NO_TOKEN. A `break` leaves a switch; a `continue` does not. */
static size_t exit_target(const KAst *ast, const KAstNode *fn, size_t p, bool want_switch) {
    size_t lo = fn->body_first - 1;
    size_t o = block_open(ast, p, lo);
    while (o != NO_TOKEN && o != lo) {
        int k = block_kind(ast, o);
        if (k == 1 || (k == 2 && want_switch)) return o;
        o = block_open(ast, o, lo);
    }
    return NO_TOKEN;
}

/* Whether the defer at token j is still pending at the exit at token p, whose
 * target block is `target` (the function's `{` for a `return`, the loop's `{`
 * for a jump): its block is target or inside it, and encloses p (j < p). */
static bool pending_at(const KAst *ast, const KAstNode *fn, size_t j, size_t p, size_t target) {
    size_t o = block_open(ast, j, fn->body_first - 1);
    if (o == NO_TOKEN || o < target) return false;
    size_t c = block_close(ast, o);
    return c == NO_TOKEN || c > p;
}

/* the number of defers that are pending at the exit at token p (see pending_at) */
static size_t pending_count(const KAst *ast, const KAstNode *fn, size_t p, size_t target) {
    size_t n = 0;
    for (size_t j = fn->body_first; j < p; j++)
        if (word(ast, j, "defer") && pending_at(ast, fn, j, p, target)) n++;
    return n;
}

/* whether token `tok` is the first token of its line (only blanks before it) */
static bool first_on_line(const KAst *ast, size_t tok) {
    const char *s = ast->source.ptr;
    size_t p = k_em_tok_off(ast, tok);
    while (p > 0 && (s[p - 1] == ' ' || s[p - 1] == '\t')) p--;
    return p == 0 || s[p - 1] == '\n';
}

/* whether the source bytes [from, to) are one line, with no comment in them */
static bool plain_span(const KAst *ast, size_t from, size_t to) {
    const char *s = ast->source.ptr;
    for (size_t p = from; p < to; p++) {
        if (s[p] == '\n') return false;
        if (s[p] == '/' && p + 1 < to && (s[p + 1] == '/' || s[p + 1] == '*')) return false;
    }
    return true;
}

/* The `;` that ends the statement starting at tok, before the body's `}`; NO_TOKEN
 * when a brace comes first (a block, or a compound literal, is not a simple statement). */
static size_t stmt_semi(const KAst *ast, const KAstNode *fn, size_t tok) {
    for (size_t j = tok; j < fn->body_end; j++) {
        if (word(ast, j, ";")) return j;
        if (word(ast, j, "{") || word(ast, j, "}")) return NO_TOKEN;
    }
    return NO_TOKEN;
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
    if (!plain_span(ast, k_em_tok_off(ast, kw), k_em_tok_end(ast, semi))) return false;
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

/* the blanks at the start of the line holding tok: their count, and the offset of the line */
static size_t line_blanks(const KAst *ast, size_t tok, size_t *start) {
    const char *s = ast->source.ptr;
    size_t p = k_em_tok_off(ast, tok), q = p;
    while (q > 0 && s[q - 1] != '\n') q--;
    size_t n = 0;
    while (q + n < p && (s[q + n] == ' ' || s[q + n] == '\t')) n++;
    *start = q;
    return n;
}

/* the .k line of token tok (1-based, as the writer counts) */
static size_t line_no(const KAst *ast, size_t tok) {
    const char *s = ast->source.ptr;
    size_t p = k_em_tok_off(ast, tok), n = 1;
    for (size_t i = 0; i < p; i++) if (s[i] == '\n') n++;
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

/* the number of defers of the body that start before token t: the ordinal of
 * the last one of them is this number minus one (keel__e<N>, backend §5.5.2) */
static size_t defers_before(const KAst *ast, const KAstNode *fn, size_t t) {
    size_t k = 0;
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->kind == K_ISLAND_DEFER && is->anchor >= fn->body_first && is->anchor < fn->body_end &&
            is->anchor < t) k++;
    }
    return k;
}

/* the first token of the return type: the visibility word, if any, is not part of it */
static size_t ret_first(const KAst *ast, const KAstNode *fn) {
    return k_em_is_visibility(k_em_tok(ast, fn->first)) ? fn->first + 1 : fn->first;
}

static bool is_void(const KAst *ast, const KAstNode *fn) {
    size_t t = ret_first(ast, fn);
    return t + 1 == fn->name_first && word(ast, t, "void");
}

/* whether the body ends in a `return` that is a statement of the body itself (the
 * natural end is then not an exit); r is the last `return` of the body */
static bool ends_in_return(const KAst *ast, const KAstNode *fn, size_t r) {
    size_t lo = fn->body_first - 1;
    if (r == NO_TOKEN || block_open(ast, r, lo) != lo) return false;
    if (!(word(ast, r - 1, ";") || word(ast, r - 1, "}") || word(ast, r - 1, "{"))) return false;
    size_t semi = stmt_semi(ast, fn, r);
    return semi != NO_TOKEN && semi + 1 == fn->body_end;
}

/* How the exits of fn are lowered (backend §5.5.2). A `return` is an exit; the
 * natural end is one too unless the body ends in a `return`. One exit is an
 * inline cleanup; two or more are the ladder. Also checks the shape of each
 * `return` for its mode. */
static Plan plan_of(const KAst *ast, const KAstNode *fn) {
    Plan p = { RET_NONE, true, false };
    size_t lo = fn->body_first - 1;
    bool any = false, nested = false;
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->kind != K_ISLAND_DEFER || is->anchor < fn->body_first || is->anchor >= fn->body_end) continue;
        any = true;
        if (block_open(ast, is->anchor, lo) != lo) nested = true;
    }
    if (!any) return p;
    size_t nret = 0, last = NO_TOKEN;
    for (size_t i = fn->body_first; i < fn->body_end; i++)
        if (word(ast, i, "return")) { nret++; last = i; }
    bool jump = has_jump(ast, fn);
    if (nret == 0 && !jump) return p;   /* no exit but the natural end: the D1 cleanup */
    if (nested && !jump) { p.ok = false; return p; }
    /* a break or continue must leave a loop (or a switch, for break) */
    for (size_t i = fn->body_first; i < fn->body_end; i++) {
        if (word(ast, i, "break") && exit_target(ast, fn, i, true) == NO_TOKEN) { p.ok = false; return p; }
        if (word(ast, i, "continue") && exit_target(ast, fn, i, false) == NO_TOKEN) { p.ok = false; return p; }
    }
    bool void_fn = is_void(ast, fn);
    p.ends_ret = nret > 0 && ends_in_return(ast, fn, last);
    if (jump || (nret == 1 && p.ends_ret)) p.mode = RET_INLINE;
    else {
        p.mode = RET_LADDER;
        if (void_fn) { p.ok = false; return p; }   /* the ladder returns keel__rv0: a value */
    }
    for (size_t r = fn->body_first; r < fn->body_end; r++) {
        if (word(ast, r, "break") || word(ast, r, "continue")) {
            /* the statement is `break;` or `continue;`, nothing else */
            if (stmt_semi(ast, fn, r) != r + 1) { p.ok = false; return p; }
            continue;
        }
        if (!word(ast, r, "return")) continue;
        size_t semi = stmt_semi(ast, fn, r);
        if (semi == NO_TOKEN) { p.ok = false; return p; }
        /* `return;` in a void function, `return expr;` in any other */
        if ((semi == r + 1) != void_fn) { p.ok = false; return p; }
        if (p.mode == RET_INLINE) {
            if (!plain_span(ast, k_em_tok_off(ast, r), k_em_tok_end(ast, semi))) { p.ok = false; return p; }
            for (size_t j = r; j <= semi; j++)     /* the expression is rebuilt from its tokens */
                if (word(ast, j, "[")) { p.ok = false; return p; }
        }
    }
    return p;
}

/* ---- text built into e->buf (the line builder; empty while edits are computed) ---- */

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

/* the return type of fn, renamed as in the C text (k_em_put_range) */
static void put_rettype(Emit *e, const KAstNode *fn) {
    k_em_put_range(e, ret_first(e->ast, fn), fn->name_first);
}

/* The body of a defer into e->buf, with the `;` that ends it: the tokens of the
 * statement, with the gaps between them as written, every name renamed as in the
 * C text (k_em_put_range); and under [now], a captured name becomes
 * keel__c<n>.<name> (not after `.` or `->`). */
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
    k_em_put_bytes(e, ";", 1);
}

/* one cleanup record of an EDIT_LINES payload: `L<line>|` for the .k line of the
 * defer at kw; the caller writes the text and then close_record */
static void open_record(Emit *e, size_t kw) {
    k_em_put_bytes(e, "L", 1);
    put_uint(e, line_no(e->ast, kw));
    k_em_put_bytes(e, "|", 1);
}

static void close_record(Emit *e) { k_em_put_bytes(e, "\n", 1); }

/* the defer's cleanup line, with the indentation of its `defer` */
static void put_cleanup(Emit *e, const KAstNode *fn, size_t kw, const Defer *d) {
    size_t ind = indent_before(e->ast, kw);
    open_record(e, kw);
    k_em_put_bytes(e, e->ast->source.ptr + k_em_tok_off(e->ast, kw) - ind, ind);
    put_body(e, d, capture_ordinal(e->ast, fn, kw));
    close_record(e);
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

/* the registration of a defer (its line becomes empty, or a struct copy for
 * [now]); with `temp`, the line also declares keel__rv0 (the ladder's temporary) */
static void edit_registration(Emit *e, const KAstNode *fn, size_t kw, const Defer *d, bool temp,
                              size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t at = k_em_tok_off(ast, kw), semi_end = k_em_tok_end(ast, d->semi);
    if (!d->now && !temp) {
        /* the statement and its indentation leave the line: the line is empty */
        size_t from = at - indent_before(ast, kw);
        drop_edits(ed, cnt, from, semi_end);
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_DROP, .at = from, .skip = semi_end - from });
        return;
    }
    /* the statement becomes the registration text, and the indentation stays */
    drop_edits(ed, cnt, at, semi_end);
    e->n = 0;
    if (d->now) {
        /* struct { T1 a; T2 b; } keel__c<N> = { a, b }; */
        k_em_put_bytes(e, "struct { ", 9);
        for (size_t c = 0; c < d->ncap; c++) {
            k_em_put_range(e, d->cap[c].type_first, d->cap[c].type_end);
            k_em_put_bytes(e, " ", 1);
            put_name(e, d->cap[c].name);
            k_em_put_bytes(e, "; ", 2);
        }
        k_em_put_bytes(e, "} keel__c", 9);
        put_uint(e, capture_ordinal(ast, fn, kw));
        k_em_put_bytes(e, " = { ", 5);
        for (size_t c = 0; c < d->ncap; c++) {
            if (c) k_em_put_bytes(e, ", ", 2);
            put_name(e, d->cap[c].name);
        }
        k_em_put_bytes(e, " };", 3);
        if (temp) k_em_put_bytes(e, " ", 1);
    }
    if (temp) {
        put_rettype(e, fn);
        k_em_put_bytes(e, " keel__rv0;", 11);
    }
    size_t len;
    char *text = take_buf(e, &len);
    if (!e->ok || !text) { e->ok = false; return; }
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = semi_end - at,
        .text = text, .text_len = len });
}

/* A `return` of a ladder function (RET_LADDER): `return expr;` becomes
 * `keel__rv0 = expr; goto keel__e<k>;`, the `return` keyword is replaced, and the
 * goto goes after the `;`. A `return` after `)` or `else` (a body without braces)
 * gets braces. k is the ordinal of the last defer registered before it. */
static void ladder_return(Emit *e, const KAstNode *fn, size_t r, size_t k, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t semi = stmt_semi(ast, fn, r);
    bool braces = word(ast, r - 1, ")") || word(ast, r - 1, "else");
    size_t at = k_em_tok_off(ast, r), end = k_em_tok_end(ast, semi);
    if (braces)
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0, .text = "{ ", .text_len = 2 });
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok(ast, r).len,
        .text = "keel__rv0 =", .text_len = 11 });
    e->n = 0;
    k_em_put_bytes(e, " goto keel__e", 13);
    put_uint(e, k);
    k_em_put_bytes(e, ";", 1);
    if (braces) k_em_put_bytes(e, " }", 2);
    size_t len;
    char *text = take_buf(e, &len);
    if (!e->ok || !text) { e->ok = false; return; }
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = end, .skip = 0, .text = text, .text_len = len });
}

/* The inline ordinal of a `return expr;` with pending defers: the number of such
 * returns before token r (the temporaries keel__rv<n> are numbered per function). */
static size_t inline_ordinal(const KAst *ast, const KAstNode *fn, size_t r) {
    size_t lo = fn->body_first - 1, n = 0;
    for (size_t i = fn->body_first; i < r; i++) {
        if (!word(ast, i, "return") || stmt_semi(ast, fn, i) == i + 1) continue;
        if (pending_count(ast, fn, i, lo) > 0) n++;
    }
    return n;
}

/* An exit that is inline (RET_INLINE, backend §5.5.2): the statement is one
 * edit, split between braces, with the cleanup in between:
 *   return;        ->  { <cleanup> return; }
 *   return expr;   ->  { T keel__rv<n> = expr; <cleanup> return keel__rv<n>; }
 *   break;         ->  { <cleanup> break; }      (and continue, the same)
 * The statement is one line and has no index (plan_of). target is the `{` the
 * exit leaves to: the function's for a return, the loop's (or switch's) for a
 * jump. The defers it runs are the pending ones (pending_at), last first. */
static void inline_exit(Emit *e, const KAstNode *fn, size_t r, size_t target, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t semi = stmt_semi(ast, fn, r);
    size_t at = k_em_tok_off(ast, r), end = k_em_tok_end(ast, semi);
    bool value = word(ast, r, "return") && semi != r + 1;   /* `return expr;` */
    size_t n = value ? inline_ordinal(ast, fn, r) : 0;
    size_t lb;
    size_t blanks = line_blanks(ast, r, &lb);
    e->n = 0;
    if (value) {
        k_em_put_bytes(e, "H|{ ", 4);
        put_rettype(e, fn);
        k_em_put_bytes(e, " keel__rv", 9);
        put_uint(e, n);
        k_em_put_bytes(e, " = ", 3);
        k_em_put_range(e, r + 1, semi);
        k_em_put_bytes(e, ";\n", 2);
    } else {
        k_em_put_bytes(e, "H|{\n", 4);
    }
    for (size_t j = r; j-- > fn->body_first; ) {       /* the pending defers, last first */
        if (!word(ast, j, "defer") || !pending_at(ast, fn, j, r, target)) continue;
        Defer d;
        if (!analyze(ast, j, &d)) { e->ok = false; return; }
        put_cleanup(e, fn, j, &d);
    }
    k_em_put_bytes(e, "T|", 2);
    k_em_put_bytes(e, ast->source.ptr + lb, blanks);
    if (value) {
        k_em_put_bytes(e, "return keel__rv", 15);
        put_uint(e, n);
        k_em_put_bytes(e, "; }", 3);
    }
    size_t len;
    char *text = take_buf(e, &len);
    if (!e->ok || !text) { e->ok = false; return; }
    if (!value) {
        /* the statement stays in the second half; the brace closes after its `;` */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_LINES, .at = at, .skip = 0, .text = text, .text_len = len });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = end, .skip = 0, .text = " }", .text_len = 2 });
    } else {
        drop_edits(ed, cnt, at, end);     /* the expression is copied again, with its renames */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_LINES, .at = at, .skip = end - at,
            .text = text, .text_len = len });
    }
}

/* The ladder of labels at the closing `}` of a RET_LADDER function (backend
 * §5.5.2): one line per defer, last registered first, `keel__e<k>: <body>`,
 * mapped to the defer's line; then the return of the value, with the
 * indentation of the first defer. The `}` follows it on the same line. */
static void close_ladder(Emit *e, const KAstNode *fn, size_t c, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t first = NO_TOKEN;
    e->n = 0;
    for (size_t j = c; j-- > fn->body_first; ) {
        if (!word(ast, j, "defer")) continue;
        Defer d;
        if (!analyze(ast, j, &d)) { e->ok = false; return; }
        open_record(e, j);
        k_em_put_bytes(e, "keel__e", 7);
        put_uint(e, defers_before(ast, fn, j));
        k_em_put_bytes(e, ": ", 2);
        put_body(e, &d, capture_ordinal(ast, fn, j));
        close_record(e);
        first = j;
    }
    if (first == NO_TOKEN) { e->ok = false; return; }
    size_t ind = indent_before(ast, first);
    k_em_put_bytes(e, "T|", 2);
    k_em_put_bytes(e, ast->source.ptr + k_em_tok_off(ast, first) - ind, ind);
    k_em_put_bytes(e, "return keel__rv0; ", 18);
    size_t len;
    char *text = take_buf(e, &len);
    if (!e->ok || !text) { e->ok = false; return; }
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_LINES, .at = k_em_tok_off(ast, c), .skip = 0,
        .text = text, .text_len = len });
}

/* The natural end of a block (D1, backend §5.5 rule 4): at its `}`, one line per
 * defer of the block that is still pending, last registered first, then the
 * `}` after the blanks it started with. */
static void close_block(Emit *e, const KAstNode *fn, size_t c, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t open = block_open(ast, c, fn->body_first - 1);
    if (open == NO_TOKEN) { e->ok = false; return; }
    e->n = 0;
    bool any = false;
    for (size_t j = c; j-- > open + 1; ) {
        if (!word(ast, j, "defer")) continue;
        Defer d;
        if (!analyze(ast, j, &d)) { e->ok = false; return; }
        if (block_open(ast, j, fn->body_first - 1) != open) continue;
        put_cleanup(e, fn, j, &d);
        any = true;
    }
    if (!any) return;
    size_t lb;
    size_t blanks = line_blanks(ast, c, &lb);
    k_em_put_bytes(e, "T|", 2);
    k_em_put_bytes(e, ast->source.ptr + lb, blanks);
    size_t len;
    char *text = take_buf(e, &len);
    if (!e->ok || !text) { e->ok = false; return; }
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_LINES, .at = k_em_tok_off(ast, c), .skip = 0,
        .text = text, .text_len = len });
}

void k_em_edits_defer(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    if (fn->body_first == NO_TOKEN || fn->body_end == NO_TOKEN) return;
    Plan p = plan_of(ast, fn);
    if (!p.ok) { e->ok = false; return; }

    /* the registrations that start on this line */
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        size_t kw = is->anchor;
        if (is->kind != K_ISLAND_DEFER || kw < fn->body_first || kw >= fn->body_end) continue;
        size_t at = k_em_tok_off(ast, kw);
        if (at < a || at >= b) continue;
        Defer d;
        if (!analyze(ast, kw, &d)) { e->ok = false; return; }
        /* the first defer declares the ladder's temporary on its line (backend §5.5, rule 5) */
        bool temp = p.mode == RET_LADDER && defers_before(ast, fn, kw) == 0;
        edit_registration(e, fn, kw, &d, temp, a, b, ed, cnt);
    }

    /* the exits that run cleanups: a `return` after a defer, and (D3) a break or continue */
    if (p.mode != RET_NONE) {
        for (size_t r = fn->body_first; r < fn->body_end && e->ok; r++) {
            bool ret = word(ast, r, "return"), brk = word(ast, r, "break"), cnt_ = word(ast, r, "continue");
            if (!ret && !brk && !cnt_) continue;
            size_t at = k_em_tok_off(ast, r);
            if (at < a || at >= b) continue;
            if (p.mode == RET_LADDER) {         /* a ladder has returns only (no jump) */
                size_t nd = defers_before(ast, fn, r);
                if (nd > 0) ladder_return(e, fn, r, nd - 1, a, b, ed, cnt);
                continue;
            }
            size_t target = ret ? fn->body_first - 1 : exit_target(ast, fn, r, brk);
            if (pending_count(ast, fn, r, target) == 0) continue;   /* nothing pending: the exit stays */
            inline_exit(e, fn, r, target, a, b, ed, cnt);
        }
    }

    /* the closing braces: the ladder at the function's `}`, the natural end of a block at its `}` */
    for (size_t c = fn->body_first; c <= fn->body_end && c < ast->tokens.len && e->ok; c++) {
        if (!word(ast, c, "}")) continue;
        size_t at = k_em_tok_off(ast, c);
        if (at < a || at >= b) continue;
        if (c == fn->body_end && p.mode == RET_INLINE && p.ends_ret) continue;   /* the exit is the return */
        if (c == fn->body_end && p.mode == RET_LADDER) close_ladder(e, fn, c, a, b, ed, cnt);
        else close_block(e, fn, c, a, b, ed, cnt);
    }
}

bool k_em_handles_defer(const KAst *ast, const KIsland *is) {
    if (is->kind != K_ISLAND_DEFER) return false;
    const KAstNode *fn = fn_of(ast, is->anchor);
    if (!fn || has_goto(ast, fn)) return false;
    Defer d;
    if (!analyze(ast, is->anchor, &d)) return false;
    size_t open = block_open(ast, is->anchor, fn->body_first - 1);
    if (open == NO_TOKEN) return false;
    size_t close = block_close(ast, open);
    if (close == NO_TOKEN || close > fn->body_end) return false;
    /* the cleanup goes before the `}`, on its line: the `}` must start the line */
    if (!first_on_line(ast, close)) return false;
    return plan_of(ast, fn).ok;
}
