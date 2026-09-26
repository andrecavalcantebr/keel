#include "engine/parser.h"

keel_slice_char k_scan_qualified_name(KLexer *lexer, KToken first,
                                       KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken last = first;
    KToken tok;

    while (true) {
        tok = k_lexer_next(lexer, next_pp_kind_out);
        if (!k_token_is_punct(tok, ".")) {
            *next_out = tok;
            break;
        }
        last = k_lexer_next(lexer, next_pp_kind_out);
    }

    return (keel_slice_char){ .ptr = first.ptr, .len = (last.ptr + last.len) - first.ptr };
}