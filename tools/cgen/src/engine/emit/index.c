/* engine/emit/index.c — the verified index of a function body (emit_internal.h;
 * codegen design E5: one file per construction; backend §5.3, rules 4 and 6).
 * An index over an `array` (`v[i]`, `m[i,j]`, `m[1][j]`) passes each index
 * through keel_index(i, d) against the declared dimension d of its position,
 * except when the index and the dimension are both decimal numbers that the
 * translator has already checked: then the bracket stays simple (rule 6). A
 * dimension is decimal when it is a literal, or the name of a constexpr whose
 * initializer is a decimal literal (block scope or module level, found here
 * from the tokens). A comma between two indices of one bracket becomes `][`.
 * Refused (k_em_handles_index answers false): a use with no declaration of its
 * array in the function, more indices than dimensions, or a declaration that
 * array.c does not lower. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define NO_TOKEN ((size_t)-1)
#define MAX_RANK 8          /* the indices of one use, and the dimensions of one array */
#define DIGITS_MAX 9        /* the length of a decimal, as the parser's own check (isl_digits) */

/* [first, end): a run of tokens */
typedef struct { size_t first, end; } Span;

/* a use `v[..][..]`: the spans of its indices, and the commas between the
 * indices of one bracket (token indices) */
typedef struct {
    Span idx[MAX_RANK];
    size_t count;
    size_t comma[MAX_RANK];
    size_t ncomma;
} Use;

/* the declaration of the array a use names: `array T name[d0, d1]` */
typedef struct {
    size_t arr;             /* the word `array`: the island's anchor */
    size_t lbr, rbr;        /* the dimension brackets: `[` and its `]` */
    Span dim[MAX_RANK];     /* the dimensions, in order */
    size_t ndims;
    bool simple;            /* no binder, no second declarator, dimensions fit */
} Decl;

static bool word(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

/* an identifier or a keyword-like word (not a number, not punctuation) */
static bool is_word(const KAst *ast, size_t i) {
    if (i >= ast->tokens.len) return false;
    KToken t = k_em_tok(ast, i);
    char c = t.len ? t.ptr[0] : 0;
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool same_text(KToken a, KToken b) {
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
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

/* the number of blanks (spaces and tabs) that follow token i, up to the next token */
static size_t blanks_after(const KAst *ast, size_t i) {
    size_t p = k_em_tok_end(ast, i), lim = k_em_tok_off(ast, i + 1);
    const char *s = ast->source.ptr;
    while (p < lim && (s[p] == ' ' || s[p] == '\t')) p++;
    return p - k_em_tok_end(ast, i);
}

/* the value of token i when it is a decimal literal (digits only) */
static bool decimal_token(const KAst *ast, size_t i, long *v) {
    if (i >= ast->tokens.len) return false;
    KToken t = k_em_tok(ast, i);
    if (t.len == 0 || t.len > DIGITS_MAX) return false;
    long n = 0;
    for (size_t k = 0; k < t.len; k++) {
        if (t.ptr[k] < '0' || t.ptr[k] > '9') return false;
        n = n * 10 + (t.ptr[k] - '0');
    }
    *v = n;
    return true;
}

static bool single_decimal(const KAst *ast, Span s, long *v) {
    return s.end == s.first + 1 && decimal_token(ast, s.first, v);
}

/* Splits [first, end) at its outer commas into `out` (at most MAX_RANK spans);
 * the commas go to `commas` when it is not NULL. False when a part is empty,
 * there are too many, or the brackets do not balance. */
static bool split_outer(const KAst *ast, size_t first, size_t end, Span *out, size_t *n,
                        size_t *commas, size_t *ncommas) {
    size_t st = first, depth = 0;
    *n = 0;
    if (ncommas) *ncommas = 0;
    for (size_t i = first; i <= end; i++) {
        if (i < end) {
            if (opens(ast, i)) { depth++; continue; }
            if (closes(ast, i)) {
                if (depth == 0) return false;
                depth--;
                continue;
            }
            if (depth != 0 || !word(ast, i, ",")) continue;
            if (commas) {
                if (*ncommas >= MAX_RANK) return false;
                commas[(*ncommas)++] = i;
            }
        }
        if (*n >= MAX_RANK || i == st) return false;
        out[(*n)++] = (Span){ st, i };
        st = i + 1;
    }
    return depth == 0;
}

/* Reads the declaration whose word `array` is token `arr`. False when the tokens
 * do not read as `array T name[...]` at all; d->simple says whether array.c lowers
 * it (no binder in the dimensions, no second declarator). */
static bool read_decl(const KAst *ast, const KAstNode *fn, size_t arr, Decl *d) {
    if (!word(ast, arr, "array")) return false;
    size_t lbr = arr + 1;
    for (;; lbr++) {
        if (lbr >= fn->end) return false;
        if (word(ast, lbr, "[")) break;
        if (word(ast, lbr, ";") || word(ast, lbr, ",") || word(ast, lbr, ")") ||
            word(ast, lbr, "{") || word(ast, lbr, "}")) return false;
    }
    if (lbr < arr + 3 || !is_word(ast, lbr - 1)) return false;   /* a type, then the name */
    size_t rbr = closing_of(ast, lbr, fn->end);
    if (rbr == NO_TOKEN) return false;
    d->arr = arr;
    d->lbr = lbr;
    d->rbr = rbr;
    d->simple = rbr != lbr + 1;
    for (size_t i = lbr + 1; i < rbr && d->simple; i++)
        if (is_word(ast, i) && is_word(ast, i + 1)) d->simple = false;   /* a binder (§5.18) */
    size_t n = 0;
    if (d->simple && !split_outer(ast, lbr + 1, rbr, d->dim, &n, NULL, NULL)) d->simple = false;
    d->ndims = n;
    if (d->simple && arr >= fn->body_first && rbr + 1 < fn->end && word(ast, rbr + 1, ","))
        d->simple = false;                                          /* a second declarator */
    return true;
}

/* The declaration of the array that the use at token `u` names: the nearest
 * `array` before it, in this function, with the same name. False when there is
 * none, or the nearest one is not a form this file lowers. */
static bool find_decl(const KAst *ast, const KAstNode *fn, size_t u, Decl *out) {
    bool found = false;
    size_t best = 0;
    Decl d;
    for (size_t k = 0; k < ast->islands.len; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        size_t arr = is->anchor;
        if (is->kind != K_ISLAND_ARRAY || arr < fn->first || arr >= u) continue;
        if (found && arr < best) continue;
        Decl c;
        if (!read_decl(ast, fn, arr, &c) || !same_text(k_em_tok(ast, c.lbr - 1), k_em_tok(ast, u))) continue;
        best = arr;
        d = c;
        found = true;
    }
    if (!found || !d.simple) return false;
    *out = d;
    return true;
}

/* 0: the constexpr at token j does not declare the name at token t; 1: it does,
 * with a decimal literal (*v is set); -1: it does, with anything else. Its
 * declaration is `constexpr T N = INIT;`, read up to `lim`. */
static int constexpr_at(const KAst *ast, size_t j, size_t lim, size_t t, long *v) {
    size_t eq = j + 1;
    while (eq < lim && !word(ast, eq, "=") && !word(ast, eq, ";")) eq++;
    if (eq >= lim || eq < j + 3 || !word(ast, eq, "=")) return 0;
    if (!is_word(ast, eq - 1) || !same_text(k_em_tok(ast, eq - 1), k_em_tok(ast, t))) return 0;
    if (decimal_token(ast, eq + 1, v) && word(ast, eq + 2, ";")) return 1;
    return -1;
}

/* The value of the name at token t, as a constexpr of a decimal literal, seen
 * from token `at` (the array's declaration): the nearest block constexpr in the
 * body before it, read as constexpr_block.c reads uses (a sibling block is
 * skipped), and otherwise the module's constexpr of that name. */
static bool constexpr_value(const KAst *ast, const KAstNode *fn, size_t at, size_t t, long *v) {
    if (at >= fn->body_first) {
        size_t depth = 0;
        for (size_t j = at; j > fn->body_first; ) {
            j--;
            if (word(ast, j, "}")) depth++;
            else if (word(ast, j, "{")) { if (depth) depth--; }
            else if (depth == 0 && word(ast, j, "constexpr")) {
                int r = constexpr_at(ast, j, fn->end, t, v);
                if (r != 0) return r > 0;
            }
        }
    }
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_CONSTEXPR || n->name_first >= n->name_end) continue;
        if (!same_text(k_em_tok(ast, n->name_first), k_em_tok(ast, t))) continue;
        size_t eq = n->name_first + 1;
        return word(ast, eq, "=") && eq + 1 < n->end && decimal_token(ast, eq + 1, v) &&
               (word(ast, eq + 2, ";") || eq + 2 == n->end);
    }
    return false;
}

/* The value of a dimension (a span of the declaration), when it is decimal. */
static bool dim_value(const KAst *ast, const KAstNode *fn, const Decl *d, Span s, long *v) {
    if (s.end != s.first + 1) return false;
    if (decimal_token(ast, s.first, v)) return true;
    if (!is_word(ast, s.first)) return false;
    return constexpr_value(ast, fn, d->arr, s.first, v);
}

/* Reads the use whose name is token u: the brackets that follow it, while they
 * are brackets (`v[i]`, `m[i,j]`, `m[1][j]`). False when they do not read. */
static bool read_use(const KAst *ast, size_t u, size_t lim, Use *x) {
    x->count = 0;
    x->ncomma = 0;
    size_t o = u + 1;
    while (word(ast, o, "[")) {
        size_t cl = closing_of(ast, o, lim);
        if (cl == NO_TOKEN) return false;
        Span tmp[MAX_RANK];
        size_t nt = 0, tc[MAX_RANK], ntc = 0;
        if (!split_outer(ast, o + 1, cl, tmp, &nt, tc, &ntc)) return false;
        if (x->count + nt > MAX_RANK) return false;
        for (size_t k = 0; k < nt; k++) x->idx[x->count++] = tmp[k];
        for (size_t k = 0; k < ntc; k++) x->comma[x->ncomma++] = tc[k];
        o = cl + 1;
    }
    return x->count > 0;
}

/* appends x when its position is in the line [a, b) */
static void push_in(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, Edit x) {
    if (x.at >= a && x.at < b) k_em_push_edit(e, ed, cnt, x);
}

void k_em_edits_index(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        size_t u = is->anchor;
        if (is->kind != K_ISLAND_ARRAY_INDEX || u < fn->first || u >= fn->end) continue;
        Use x;
        Decl d;
        if (!read_use(ast, u, fn->end, &x) || !find_decl(ast, fn, u, &d) || x.count > d.ndims) continue;

        /* the indices: each one passes through keel_index, unless it and its
         * dimension are both known decimals (rule 6) */
        for (size_t j = 0; j < x.count; j++) {
            long iv = 0, dv = 0;
            if (single_decimal(ast, x.idx[j], &iv) && dim_value(ast, fn, &d, d.dim[j], &dv) && iv < dv)
                continue;
            Span ix = x.idx[j];
            size_t open_at = k_em_tok_off(ast, ix.first);
            size_t close_at = k_em_tok_end(ast, ix.end - 1);
            push_in(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = open_at, .skip = 0,
                .text = "keel_index(", .text_len = 11 });
            push_in(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = close_at, .skip = 0,
                .text = ", ", .text_len = 2 });
            push_in(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TOKS, .at = close_at, .skip = 0,
                .tok_a = d.dim[j].first, .tok_b = d.dim[j].end });
            push_in(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = close_at, .skip = 0,
                .text = ")", .text_len = 1 });
        }

        /* the commas between indices of one bracket become `][` (`m[i,j]` is
         * `m[i][j]`), whatever the indices are */
        for (size_t c = 0; c < x.ncomma; c++)
            push_in(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, x.comma[c]),
                .skip = 1 + blanks_after(ast, x.comma[c]), .text = "][", .text_len = 2 });
    }
}

bool k_em_handles_index(const KAst *ast, const KIsland *is) {
    if (is->kind != K_ISLAND_ARRAY_INDEX || is->anchor >= ast->tokens.len) return false;
    const KAstNode *fn = enclosing_fn(ast, is->anchor);
    if (!fn) return false;
    Use x;
    Decl d;
    return read_use(ast, is->anchor, fn->end, &x) && find_decl(ast, fn, is->anchor, &d) && x.count <= d.ndims;
}
