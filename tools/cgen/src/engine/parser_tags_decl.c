#include "engine/parser.h"

bool k_scan_tags_decl(KLexer *lexer, KToken tags_kw, KSymbolTable *symtab,
                       KTagsDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    (void)tags_kw;
    out->name = k_lexer_next(lexer, next_pp_kind_out);
    out->item_count = 0;
    out->has_values = false;
    out->all_values = true;

    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_punct(tok, "[")) return false;
    tok = k_lexer_next(lexer, next_pp_kind_out);

    while (!k_token_is_punct(tok, "]")) {
        if (tok.len == 0 || !k_token_is_ident(tok)) return false;
        if (out->item_count >= K_TAGS_MAX_ITEMS) return false;
        size_t at = out->item_count++;
        out->items[at] = tok;
        out->values[at] = (keel_slice_char){ 0, NULL };

        tok = k_lexer_next(lexer, next_pp_kind_out);
        if (k_token_is_punct(tok, "=")) {
            /* tag-value ::= [ '-' ] ( NUM | qualified-name ) */
            out->has_values = true;
            tok = k_lexer_next(lexer, next_pp_kind_out);
            const char *from = tok.ptr;
            const char *to;
            if (k_token_is_punct(tok, "-")) tok = k_lexer_next(lexer, next_pp_kind_out);
            if (k_token_is_ident(tok)) {
                keel_slice_char name = k_scan_qualified_name(lexer, tok, &tok,
                                                             next_pp_kind_out);
                to = name.ptr + name.len;
            } else if (k_token_is_number(tok)) {
                to = tok.ptr + tok.len;
                tok = k_lexer_next(lexer, next_pp_kind_out);
            } else {
                return false;
            }
            out->values[at] = (keel_slice_char){ (size_t)(to - from), (char *)from };
        } else {
            out->all_values = false;
        }

        if (k_token_is_punct(tok, ",")) {
            tok = k_lexer_next(lexer, next_pp_kind_out);
            continue;
        }
        if (!k_token_is_punct(tok, "]")) return false;
    }

    tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_punct(tok, ";")) return false;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return k_symtab_insert(symtab, out->name, K_SYM_TAGS, 0);
}
