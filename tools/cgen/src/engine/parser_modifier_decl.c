#include "engine/parser.h"

bool k_scan_modifier_decl(KLexer *lexer, KToken modifier_kw, int module_arity,
                           KSymbolTable *symtab, KModifierDecl *out,
                           KToken *next_out, TKPpKind *next_pp_kind_out) {
    out->name = k_lexer_next(lexer, next_pp_kind_out);
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    out->byref = false;

    if (k_token_is_ident_named(tok, "byref")) {
        out->byref = true;
        tok = k_lexer_next(lexer, next_pp_kind_out);
    }

    out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);
    return k_symtab_insert(symtab, out->name, K_SYM_MODIFIER, module_arity);
}