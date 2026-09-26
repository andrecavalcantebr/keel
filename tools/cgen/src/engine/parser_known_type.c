#include "engine/parser.h"

bool k_scan_known_type(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                        KSpecifier *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    const KSymbol *sym = k_symtab_lookup(symtab, first);
    
    if (sym == NULL) {
        out->kind = K_SPEC_NONE;
        return true;
    }
    
    if (sym->kind == K_SYM_MODIFIER) {
        out->kind = K_SPEC_MODIFIER;
        out->modifier_name = first;
        out->arg_count = 0;
        
        for (size_t i = 0; i < sym->arity; ++i) {
            if (i >= 4) {
                return false;
            }
            out->args[i] = k_lexer_next(lexer, next_pp_kind_out);
            out->arg_count++;
        }
        
        *next_out = k_lexer_next(lexer, next_pp_kind_out);
        return true;
    }
    
    if (sym->kind == K_SYM_TYPE) {
        out->kind = K_SPEC_NAMED_TYPE;
        out->type_name = first;
        *next_out = k_lexer_next(lexer, next_pp_kind_out);
        return true;
    }
    
    out->kind = K_SPEC_NONE;
    return true;
}