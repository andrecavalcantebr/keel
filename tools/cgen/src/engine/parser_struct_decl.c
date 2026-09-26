#include "engine/parser.h"

bool k_scan_struct_decl(KLexer *lexer, KToken struct_or_union_kw, KSymbolTable *symtab,
                         KStructDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    out->is_union = k_token_is_c_word_named(struct_or_union_kw, "union");
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);

    if (k_token_is_ident(tok)) {
        out->tag_name = tok;
        tok = k_lexer_next(lexer, next_pp_kind_out);
    } else {
        out->tag_name = (keel_slice_char){0};
    }

    out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);
    *next_out = k_lexer_next(lexer, next_pp_kind_out);

    if (out->tag_name.len != 0) {
        return k_symtab_insert(symtab, out->tag_name, K_SYM_TYPE, 0);
    }
    return true;
}