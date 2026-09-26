#include "engine/parser.h"

bool k_scan_declarator_head(KLexer *lexer, KToken first, KDeclaratorHead *out,
                             KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken tok = first;
    out->pointer_depth = 0;

    while (k_token_is_punct(tok, "*")) {
        out->pointer_depth++;
        tok = k_lexer_next(lexer, next_pp_kind_out);
    }

    out->name = tok;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return true;
}