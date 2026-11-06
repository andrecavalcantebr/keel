#include "engine/parser/parser.h"

/* ---- k_scan_opaque_until ---- */

void k_scan_opaque_until(KLexer *lexer, KToken first, const char *const *terminators,
                          size_t terminator_count, size_t *terminator_index_out,
                          KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken tok = first;
    KGroupDepth gd;
    k_group_depth_init(&gd, K_GROUP_KIND_ANY);
    TKPpKind pp = TK_PP_OTHER;      /* of `tok`: `first` is a token already read */
    (void)pp;

    for (;;) {
        if (gd.depth == 0) {
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

        k_group_depth_feed(&gd, tok, pp);
        tok = k_lexer_next(lexer, next_pp_kind_out);
        pp = *next_pp_kind_out;
    }
}

/* ---- k_scan_braced_opaque ---- */

keel_slice_char k_scan_braced_opaque(KLexer *lexer, KToken open,
                                      KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken first;
    KToken last;
    bool seen_content = false;
    KGroupDepth gd;
    k_group_depth_init(&gd, K_GROUP_KIND_BRACE);
    gd.depth = 1;

    while (true) {
        TKPpKind pp;
        KToken tok = k_lexer_next(lexer, &pp);
        *next_pp_kind_out = pp;
        if (tok.len == 0) { *next_out = tok; break; }       /* end of file: the delimiter pass has told */
        k_group_depth_feed(&gd, tok, pp);
        /* the body ends at the `}` that leaves no depth and no group open */
        if (k_token_is_punct(tok, "}") && gd.depth == 0 && gd.groups == 0) {
            *next_out = k_lexer_next(lexer, next_pp_kind_out);
            break;
        }
        if (!seen_content) {
            first = tok;
            seen_content = true;
        }
        last = tok;
    }

    if (!seen_content) {
        return (keel_slice_char){ .ptr = open.ptr + open.len, .len = 0 };
    }

    return (keel_slice_char){ .ptr = first.ptr, .len = (size_t)((last.ptr + last.len) - first.ptr) };
}

/* ---- k_scan_qualified_name ---- */

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

/* ---- k_scan_ident_list ---- */

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
