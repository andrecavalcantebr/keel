#include "engine/parser.h"

bool k_scan_ident_list(KLexer *lexer, KToken first, KToken *out, size_t cap,
                        size_t *count_out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    size_t count = 0;
    out[count++] = first;

    while (true) {
        KToken tok = k_lexer_next(lexer, next_pp_kind_out);
        if (!k_token_is_punct(tok, ",")) {
            *next_out = tok;
            break;
        }

        KToken ident = k_lexer_next(lexer, next_pp_kind_out);
        if (count == cap) {
            *count_out = count;
            return false;
        }
        out[count++] = ident;
    }

    *count_out = count;
    return true;
}