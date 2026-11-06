/* engine/emit/defer_scope.c — the scope and jump target of a token inside a
 * function's body (backend §5.5, §4.6 rule 9), the structural analysis `defer.c`
 * builds its plan and its lowering on: the `{` that opens the block holding a
 * token, the `}` that closes it, the loop or switch a `break`/`continue` leaves,
 * and the block a `goto`'s label is in. keel-spec §2.4's groups count as one
 * block each (KGroupDepth), not one per alternative. No I/O, no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/lexer/lexer.h"

#define NO_TOKEN ((size_t)-1)

static bool word(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

/* an identifier or a keyword-like word (a type name, a binder), not a number */
bool k_em_word_like(const KAst *ast, size_t i) {
    if (i >= ast->tokens.len) return false;
    KToken t = k_em_tok(ast, i);
    char c = t.len ? t.ptr[0] : 0;
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool k_em_same_text(KToken a, KToken b) {
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
}

/* the `]` that closes the `[` at `open`, or NO_TOKEN */
size_t k_em_close_square(const KAst *ast, size_t open) {
    size_t depth = 0;
    for (size_t i = open; i < ast->tokens.len; i++) {
        if (word(ast, i, "[")) depth++;
        else if (word(ast, i, "]") && --depth == 0) return i;
    }
    return NO_TOKEN;
}

/* The function whose body holds token tok, or NULL. */
const KAstNode *k_em_fn_of(const KAst *ast, size_t tok) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind == K_AST_FUNCTION && n->body_first != NO_TOKEN && n->body_end != NO_TOKEN &&
            tok >= n->body_first && tok < n->body_end) return n;
    }
    return NULL;
}

/* an identifier token (a label's name) */
static bool ident(const KAst *ast, size_t i) {
    return i < ast->tokens.len && k_token_is_ident(k_em_tok(ast, i));
}

/* The `{` that opens the block holding token `from`, searched back from from-1
 * down to `lo` (the function's own `{` is body_first - 1); NO_TOKEN if none.
 * `lo` is always outside any conditional group open at `from` — the function's
 * own `{` is a scanner with no group left to resolve — so a stack of the `{`
 * seen, built going FORWARD from `lo` with KGroupDepth, collapses a group of
 * keel-spec §2.4 between `lo` and `from` to one opening apiece instead of
 * adding every alternative's on top of each other (the naive backward count
 * this replaced did exactly that, and refused valid sources with
 * implementation-limit when a `defer` crossed one). */
#define K_DEFER_BRACE_STACK_MAX 256
size_t k_em_block_open(const KAst *ast, size_t from, size_t lo) {
    KGroupDepth gd;
    k_group_depth_init(&gd, K_GROUP_KIND_BRACE);
    size_t stack[K_DEFER_BRACE_STACK_MAX];
    int top = 0;
    for (size_t j = lo; j < from; j++) {
        const KLexeme *l = keel_buffer_KLexeme_ptr(&ast->tokens, j);
        int before = gd.depth;
        k_group_depth_feed(&gd, l->token, l->pp_kind);
        while (gd.depth > before && top < K_DEFER_BRACE_STACK_MAX) { stack[top++] = j; before++; }
        while (gd.depth < before && top > 0) { top--; before--; }
    }
    return top > 0 ? stack[top - 1] : NO_TOKEN;
}

/* The `}` that closes the block opened at `open`, or NO_TOKEN. */
size_t k_em_block_close(const KAst *ast, size_t open) {
    KGroupDepth gd;
    k_group_depth_init(&gd, K_GROUP_KIND_BRACE);
    for (size_t j = open; j < ast->tokens.len; j++) {
        const KLexeme *l = keel_buffer_KLexeme_ptr(&ast->tokens, j);
        k_group_depth_feed(&gd, l->token, l->pp_kind);
        if (gd.depth == 0 && word(ast, j, "}")) return j;
    }
    return NO_TOKEN;
}

/* What the `{` at open opens: 1 a loop body (`for (...)`, `while (...)`, `foreach (...)`, `do`),
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
            if (word(ast, q - 1, "for") || word(ast, q - 1, "while") || word(ast, q - 1, "foreach")) return 1;
            if (word(ast, q - 1, "switch")) return 2;
            return 0;
        }
    }
    return 0;
}

/* The `{` of the innermost loop (or switch, when want_switch) that holds token p,
 * or NO_TOKEN. A `break` leaves a switch; a `continue` does not. */
size_t k_em_exit_target(const KAst *ast, const KAstNode *fn, size_t p, bool want_switch) {
    size_t lo = fn->body_first - 1;
    size_t o = k_em_block_open(ast, p, lo);
    while (o != NO_TOKEN && o != lo) {
        int k = block_kind(ast, o);
        if (k == 1 || (k == 2 && want_switch)) return o;
        o = k_em_block_open(ast, o, lo);
    }
    return NO_TOKEN;
}

/* whether the defer at token j was registered in the function's own scope: its
 * cleanup is the one the ladder can carry (backend §5.5.2, rule 6) */
bool k_em_at_fn_scope(const KAst *ast, const KAstNode *fn, size_t j) {
    size_t lo = fn->body_first - 1;
    return k_em_block_open(ast, j, lo) == lo;
}

/* The `{` of the block holding the label a `goto` names, or NO_TOKEN when the
 * label is not in the function or its block does not enclose the `goto`: the
 * jump would enter a scope, which the language refuses (spec §4.6, rule 9). The
 * label is `<name> :` at the start of a statement. */
size_t k_em_goto_target(const KAst *ast, const KAstNode *fn, size_t g) {
    if (!word(ast, g + 1, ";") && !ident(ast, g + 1)) return NO_TOKEN;
    KToken name = k_em_tok(ast, g + 1);
    if (name.len == 0 || !word(ast, g + 2, ";")) return NO_TOKEN;
    size_t lo = fn->body_first - 1, label = NO_TOKEN;
    for (size_t i = fn->body_first; i < fn->body_end; i++) {
        KToken t = k_em_tok(ast, i);
        if (t.len != name.len || memcmp(t.ptr, name.ptr, t.len) != 0) continue;
        if (!word(ast, i + 1, ":")) continue;
        if (!(word(ast, i - 1, ";") || word(ast, i - 1, "{") || word(ast, i - 1, "}") || word(ast, i - 1, ":")))
            continue;
        label = i;
        break;
    }
    if (label == NO_TOKEN) return NO_TOKEN;
    size_t o = k_em_block_open(ast, label, lo);
    if (o == NO_TOKEN) return NO_TOKEN;
    /* the label's block must enclose the goto: walk the goto's blocks out to it */
    for (size_t q = k_em_block_open(ast, g, lo); q != NO_TOKEN; q = k_em_block_open(ast, q, lo)) {
        if (q == o) return o;
        if (q == lo) break;
    }
    return o == lo ? lo : NO_TOKEN;
}
