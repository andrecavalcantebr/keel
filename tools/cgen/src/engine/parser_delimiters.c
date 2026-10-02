/* The structural consumer of lexer-design §7 and keel-spec §2.4: `()`, `[]`
 * and `{}` balance in every alternative of a conditional group, and all the
 * alternatives of a group leave the same openings behind. The lexer only
 * delimits; this pass pairs. */
#include <string.h>
#include "engine/ast.h"
#include "engine/parser.h"

#define K_OPEN_MAX 256
#define K_GROUP_MAX 32

typedef struct {
    size_t entry[K_OPEN_MAX], entry_n;      /* the openings when the group began */
    size_t result[K_OPEN_MAX], result_n;    /* those the first alternative left */
    bool has_result, has_else, told;
    size_t at;                              /* the `#if` */
} Group;

static char kind_of(KToken t) {
    if (k_token_is_punct(t, "(") || k_token_is_punct(t, ")")) return '(';
    if (k_token_is_punct(t, "[") || k_token_is_punct(t, "]")) return '[';
    if (k_token_is_punct(t, "{") || k_token_is_punct(t, "}")) return '{';
    return 0;
}

static KToken token(const KAst *a, size_t i) { return keel_buffer_KLexeme_ptr(&a->tokens, i)->token; }

/* two stacks of openings agree when they open the same kinds, in order */
static bool same(const KAst *a, const size_t *x, size_t xn, const size_t *y, size_t yn) {
    if (xn != yn) return false;
    for (size_t i = 0; i < xn; i++) if (kind_of(token(a, x[i])) != kind_of(token(a, y[i]))) return false;
    return true;
}

static void finish(const KAst *a, Group *g, const size_t *open, size_t n, KDiagnosticSink *diag) {
    if (!g->has_result) {
        memcpy(g->result, open, n * sizeof *open);
        g->result_n = n;
        g->has_result = true;
    } else if (!g->told && !same(a, g->result, g->result_n, open, n)) {
        KToken at = token(a, g->at);
        k_diag_emit(diag, K_DIAG_DELIMITER_MISMATCH_ACROSS_BRANCHES, at, (KDiagArgs){{ at }});
        g->told = true;
    }
}

bool k_check_delimiters(const KAst *a, KDiagnosticSink *diag) {
    static size_t open[K_OPEN_MAX];
    static Group groups[K_GROUP_MAX];
    size_t n = 0, ng = 0;
    bool ok = true;
    for (size_t i = 0; i < a->tokens.len; i++) {
        const KLexeme *l = keel_buffer_KLexeme_ptr(&a->tokens, i);
        if (l->pp_kind == TK_PP_IF) {
            if (ng == K_GROUP_MAX) continue;
            Group *g = &groups[ng++];
            memcpy(g->entry, open, n * sizeof *open);
            g->entry_n = n;
            g->has_result = g->has_else = g->told = false;
            g->at = i;
            continue;
        }
        if (l->pp_kind == TK_PP_ELSE && ng) {
            Group *g = &groups[ng - 1];
            finish(a, g, open, n, diag);
            if (k_token_spelled(l->token, "#else") || strstr(l->token.ptr, "else") == l->token.ptr + 1) g->has_else = true;
            memcpy(open, g->entry, g->entry_n * sizeof *open);
            n = g->entry_n;
            continue;
        }
        if (l->pp_kind == TK_PP_ENDIF && ng) {
            Group *g = &groups[--ng];
            finish(a, g, open, n, diag);
            if (!g->has_else) finish(a, g, g->entry, g->entry_n, diag);     /* the empty alternative */
            if (g->told) ok = false;
            memcpy(open, g->result, g->result_n * sizeof *open);
            n = g->result_n;
            continue;
        }
        if (l->directive) continue;
        char k = kind_of(l->token);
        if (!k) continue;
        if (k_token_is_punct(l->token, "(") || k_token_is_punct(l->token, "[") || k_token_is_punct(l->token, "{")) {
            if (n < K_OPEN_MAX) open[n++] = i;
            continue;
        }
        if (!n) {
            k_diag_emit(diag, K_DIAG_UNMATCHED_DELIMITER, l->token, (KDiagArgs){{ l->token, k_diag_text("nothing") }});
            ok = false;
            continue;
        }
        if (kind_of(token(a, open[n - 1])) != k) {
            KToken opener = token(a, open[n - 1]);
            k_diag_emit(diag, K_DIAG_UNMATCHED_DELIMITER, l->token, (KDiagArgs){{ l->token, opener }});
            ok = false;
        }
        n--;
    }
    for (size_t i = 0; i < n; i++) {
        KToken t = token(a, open[i]);
        k_diag_emit(diag, K_DIAG_UNMATCHED_DELIMITER, t, (KDiagArgs){{ t, k_diag_text("the end of the file") }});
        ok = false;
    }
    return ok;
}
