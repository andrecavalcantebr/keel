#include "engine/parser.h"

void k_scan_opaque_until(KLexer *lexer, KToken first, const char *const *terminators,
                          size_t terminator_count, size_t *terminator_index_out,
                          KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken tok = first;
    int depth = 0;

    for (;;) {
        if (depth == 0) {
            for (size_t i = 0; i < terminator_count; ++i) {
                if (k_token_is_punct(tok, terminators[i])) {
                    *terminator_index_out = i;
                    *next_out = tok;
                    return;
                }
            }
        }

        if (tok.len == 0) {
            *terminator_index_out = 0;
            *next_out = tok;
            return;
        }

        if (k_token_is_punct(tok, "(") || k_token_is_punct(tok, "[") || k_token_is_punct(tok, "{")) {
            depth++;
        } else if (k_token_is_punct(tok, ")") || k_token_is_punct(tok, "]") || k_token_is_punct(tok, "}")) {
            depth--;
        }

        tok = k_lexer_next(lexer, next_pp_kind_out);
    }
}