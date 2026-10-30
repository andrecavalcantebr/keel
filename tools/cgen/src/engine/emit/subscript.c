/* engine/emit/subscript.c — the index sugar over a container (emit_internal.h;
 * codegen design E5: one file per construction; backend §5.3, rules 1 and 7).
 * `x[i]` over a modifier instance is `*ptr(x, i)`, and `x[a..b]` with its open
 * forms is `as_slice(x, a, b)`, the open ends reading `length(x)`. The island's
 * detail gives the C symbols: `<name> → <ptr>` (an index), or
 * `<name> <a..b> → <as_slice> [<length>]` (a range-index), and a trailing `&1`
 * when the container goes by address. The indices and the bounds stay as
 * written; the edits wrap the container's name and replace the brackets.
 * Refused (k_em_handles_subscript answers false): a container that is not a
 * plain name (`s.f[i]`), a comma between indices (§5.3.1), a name preceded by
 * `&` (rule 2 collapses `&*`, not lowered yet), a bracket group over two lines,
 * and an island outside a function body. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define NO_TOKEN ((size_t)-1)
#define NO_WORDS ((size_t)-1)
#define MAX_WORDS 3             /* the detail's words after the arrow: symbol, length, &1 */
#define ARROW "\xe2\x86\x92"    /* the UTF-8 of the arrow the parser prints */

/* one word of a detail: the bytes [p, p + n) of island_text */
typedef struct { const char *p; size_t n; } Word;

/* the bracket group after a container's name, and what its island says */
typedef struct {
    size_t open, close;         /* the `[` (the name's next token) and its `]` */
    size_t dots;                /* range-index: its `..`; NO_TOKEN for an index */
    bool low, high;             /* range-index: a bound before the `..`, and after it */
    bool addr;                  /* the detail's `&1`: the container goes by address */
    Word verb, length;          /* the symbol, and the `length` of an open end */
} Sugar;

static bool word(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

static bool opens(const KAst *ast, size_t i) {
    return word(ast, i, "(") || word(ast, i, "[") || word(ast, i, "{");
}

static bool closes(const KAst *ast, size_t i) {
    return word(ast, i, ")") || word(ast, i, "]") || word(ast, i, "}");
}

/* the token that closes the bracket at `open`, before `lim`, or NO_TOKEN */
static size_t closing_of(const KAst *ast, size_t open, size_t lim) {
    size_t depth = 0;
    for (size_t i = open; i < lim && i < ast->tokens.len; i++) {
        if (opens(ast, i)) depth++;
        else if (closes(ast, i) && --depth == 0) return i;
    }
    return NO_TOKEN;
}

/* the function node whose tokens hold token `tok`, or NULL */
static const KAstNode *enclosing_fn(const KAst *ast, size_t tok) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind == K_AST_FUNCTION && n->first <= tok && tok < n->end) return n;
    }
    return NULL;
}

static bool one_line(const KAst *ast, size_t from, size_t to) {
    for (size_t p = from; p < to; p++)
        if (ast->source.ptr[p] == '\n') return false;
    return true;
}

/* The words after the arrow of the island's detail, into w. Returns their
 * number, or NO_WORDS when there is no arrow or more than MAX_WORDS words. */
static size_t detail_words(const KAst *ast, const KIsland *is, Word *w) {
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, p = 0, n = 0;
    while (p + 3 <= len && memcmp(d + p, ARROW, 3) != 0) p++;
    if (p + 3 > len) return NO_WORDS;
    p += 3;
    while (p < len) {
        while (p < len && d[p] == ' ') p++;
        if (p >= len) break;
        size_t s = p;
        while (p < len && d[p] != ' ') p++;
        if (n == MAX_WORDS) return NO_WORDS;
        w[n++] = (Word){ d + s, p - s };
    }
    return n;
}

static bool is_addr(Word w) {
    return w.n == 2 && w.p[0] == '&' && w.p[1] == '1';
}

/* Reads the island at `is` as one of the two forms this file lowers: an index
 * over a plain name (`x[i]`) or a range-index (`x[a..b]`, `x[a..]`, `x[..b]`,
 * `x[..]`). False for any other island, and for a form this file refuses. */
static bool read_sugar(const KAst *ast, const KIsland *is, Sugar *s) {
    bool range = is->kind == K_ISLAND_RANGE_INDEX;
    size_t t = is->anchor;
    if (!range && is->kind != K_ISLAND_INDEX) return false;
    if (!word(ast, t + 1, "[") || (t > 0 && word(ast, t - 1, "&"))) return false;
    const KAstNode *fn = enclosing_fn(ast, t);
    if (!fn) return false;
    size_t close = closing_of(ast, t + 1, fn->end);
    if (close == NO_TOKEN || !one_line(ast, k_em_tok_off(ast, t), k_em_tok_end(ast, close))) return false;

    /* one index only (§5.3.1 is the comma form); the `..` of a range-index, and
     * no `..` in an index */
    size_t dots = NO_TOKEN, depth = 0;
    for (size_t i = t + 2; i < close; i++) {
        if (opens(ast, i)) depth++;
        else if (closes(ast, i)) depth--;
        else if (depth == 0 && word(ast, i, ",")) return false;
        else if (depth == 0 && word(ast, i, "..") && dots == NO_TOKEN) dots = i;
    }
    if (range != (dots != NO_TOKEN)) return false;

    Word w[MAX_WORDS];
    size_t nw = detail_words(ast, is, w);
    if (nw == NO_WORDS || nw == 0) return false;
    s->addr = nw > 1 && is_addr(w[nw - 1]);
    if (s->addr) nw--;
    s->verb = w[0];
    s->open = t + 1;
    s->close = close;
    s->dots = dots;
    s->low = range && dots > t + 2;
    s->high = range && close > dots + 1;
    /* an index: the symbol; a range-index: the symbol, and `length` when the end is open */
    size_t want = !range || s->high ? 1 : 2;
    if (nw != want) return false;
    if (range && !s->high) s->length = w[1];
    return true;
}

bool k_em_handles_subscript(const KAst *ast, const KIsland *is) {
    Sugar s;
    return read_sugar(ast, is, &s);
}

/* appends x when its position is in the line [a, b) */
static void push_in(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, Edit x) {
    if (x.at >= a && x.at < b) k_em_push_edit(e, ed, cnt, x);
}

static void put_text(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, size_t at, size_t skip,
                     const char *text, size_t len) {
    push_in(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = skip, .text = text, .text_len = len });
}

void k_em_edits_subscript(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->anchor < fn->first || is->anchor >= fn->end) continue;
        Sugar s;
        if (!read_sugar(ast, is, &s)) continue;

        size_t t = is->anchor;
        size_t name = k_em_tok_off(ast, t);
        size_t open = k_em_tok_off(ast, s.open), close = k_em_tok_off(ast, s.close);

        if (is->kind == K_ISLAND_INDEX) {
            /* x[i] is *ptr(x, i): the call opens at the name, the `[` is the comma, the `]` the paren */
            put_text(e, ed, cnt, a, b, name, 0, "*", 1);
            put_text(e, ed, cnt, a, b, name, 0, s.verb.p, s.verb.n);
            put_text(e, ed, cnt, a, b, name, 0, "(", 1);
            if (s.addr) put_text(e, ed, cnt, a, b, name, 0, "&", 1);
            put_text(e, ed, cnt, a, b, open, 1, ", ", 2);
            put_text(e, ed, cnt, a, b, close, 1, ")", 1);
            continue;
        }

        /* x[a..b] is as_slice(x, a, b); an open start is 0, an open end is length(x) */
        put_text(e, ed, cnt, a, b, name, 0, s.verb.p, s.verb.n);
        put_text(e, ed, cnt, a, b, name, 0, "(", 1);
        if (s.addr) put_text(e, ed, cnt, a, b, name, 0, "&", 1);
        if (s.low) put_text(e, ed, cnt, a, b, open, 1, ", ", 2);
        else put_text(e, ed, cnt, a, b, open, 1, ", 0", 3);
        size_t dots = k_em_tok_off(ast, s.dots), dots_end = k_em_tok_end(ast, s.dots);
        put_text(e, ed, cnt, a, b, dots, 2, ", ", 2);
        if (!s.high) {
            /* the open end: `length(x)`, with the container's name copied as written */
            put_text(e, ed, cnt, a, b, dots_end, 0, s.length.p, s.length.n);
            put_text(e, ed, cnt, a, b, dots_end, 0, "(", 1);
            if (s.addr) put_text(e, ed, cnt, a, b, dots_end, 0, "&", 1);
            push_in(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TOKS, .at = dots_end, .skip = 0, .tok_a = t, .tok_b = t + 1 });
            put_text(e, ed, cnt, a, b, close, 1, "))", 2);
        } else {
            put_text(e, ed, cnt, a, b, close, 1, ")", 1);
        }
    }
}
