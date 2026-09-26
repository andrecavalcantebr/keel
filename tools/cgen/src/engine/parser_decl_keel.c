#include "engine/parser.h"

bool k_scan_decl_keel(KLexer *lexer, KToken first, KSymbolTable *symtab,
                       KKeelDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    bool ok = k_scan_known_type(lexer, first, symtab, &out->spec, next_out, next_pp_kind_out);
    if (!ok) return false;
    if (out->spec.kind == K_SPEC_NONE) return false;

    out->name_count = 0;
    while (true) {
        KDeclaratorHead dh;
        k_scan_declarator_head(lexer, *next_out, &dh, next_out, next_pp_kind_out);
        if (out->name_count == 8) return false;
        out->names[out->name_count++].name = dh.name;
        k_symtab_insert(symtab, dh.name, K_SYM_VARIABLE, 0);

        if (k_token_is_punct(*next_out, "=")) {
            KToken init_first = k_lexer_next(lexer, next_pp_kind_out);
            static const char *terms[] = { ",", ";" };
            size_t idx;
            k_scan_opaque_until(lexer, init_first, terms, 2, &idx, next_out, next_pp_kind_out);
        }

        if (k_token_is_punct(*next_out, ",")) {
            *next_out = k_lexer_next(lexer, next_pp_kind_out);
        } else {
            break;
        }
    }

    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return true;
}