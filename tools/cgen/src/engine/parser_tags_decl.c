#include "engine/parser.h"

bool k_scan_tags_decl(KLexer *lexer, KToken tags_kw, KSymbolTable *symtab,
                       KTagsDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    out->name = k_lexer_next(lexer, next_pp_kind_out);
    k_lexer_next(lexer, next_pp_kind_out);
    KToken first_item = k_lexer_next(lexer, next_pp_kind_out);
    KToken tok;
    k_scan_ident_list(lexer, first_item, out->items, 16, &out->item_count, &tok, next_pp_kind_out);
    KToken semicolon = k_lexer_next(lexer, next_pp_kind_out);
    (void)semicolon;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return k_symtab_insert(symtab, out->name, K_SYM_TAGS, 0);
}