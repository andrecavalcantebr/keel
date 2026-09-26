#include "engine/parser.h"

keel_slice_char k_scan_braced_opaque(KLexer *lexer, KToken open,
                                      KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken first;
    KToken last;
    bool seen_content = false;
    int depth = 1;

    while (true) {
        KToken tok = k_lexer_next(lexer, next_pp_kind_out);
        if (k_token_is_punct(tok, "{")) {
            depth++;
            if (!seen_content) {
                first = tok;
                seen_content = true;
            }
            last = tok;
        } else if (k_token_is_punct(tok, "}")) {
            depth--;
            if (depth == 0) {
                *next_out = k_lexer_next(lexer, next_pp_kind_out);
                break;
            }
            if (!seen_content) {
                first = tok;
                seen_content = true;
            }
            last = tok;
        } else {
            if (!seen_content) {
                first = tok;
                seen_content = true;
            }
            last = tok;
        }
    }

    if (!seen_content) {
        return (keel_slice_char){ .ptr = open.ptr + open.len, .len = 0 };
    }

    return (keel_slice_char){ .ptr = first.ptr, .len = (size_t)((last.ptr + last.len) - first.ptr) };
}

void k_scan_extern_c(KLexer *lexer, KToken extern_c_kw, KExternCDecl *out,
                      KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    out->has_type_h = false;

    if (k_token_is_punct(tok, "[")) {
        out->has_type_h = true;
        k_lexer_next(lexer, next_pp_kind_out); // type_h
        k_lexer_next(lexer, next_pp_kind_out); // ]
        tok = k_lexer_next(lexer, next_pp_kind_out);
    }

    out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);
}