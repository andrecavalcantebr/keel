#include "engine/parser.h"

bool k_scan_decl_keel(KLexer *lexer, KToken first, KSymbolTable *symtab,
                       KKeelDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    if (!k_scan_known_type(lexer, first, symtab, &out->spec, next_out, next_pp_kind_out))
        return false;
    if (out->spec.kind == K_SPEC_NONE) return false;

    out->name_count = 0;
    for (;;) {
        /* The whole `declarator`, not just its head: `arena a[4];` ends
           its declarator at the ']', and reading only `{ '*' } IDENT`
           left *next_out on the '4' inside the brackets — reported as a
           success, which is the failure mode that matters. */
        KDeclarator declarator;
        if (!k_scan_declarator(lexer, *next_out, &declarator, next_out, next_pp_kind_out))
            return false;
        if (out->name_count == 8) return false;
        out->names[out->name_count++].name = declarator.name;
        if (!k_symtab_insert(symtab, declarator.name, K_SYM_VARIABLE, 0)) return false;

        if (k_token_is_punct(*next_out, "=")) {
            KToken init_first = k_lexer_next(lexer, next_pp_kind_out);
            static const char *const terms[] = { ",", ";" };
            size_t idx;
            k_scan_opaque_until(lexer, init_first, terms, 2, &idx, next_out, next_pp_kind_out);
        }

        if (k_token_is_punct(*next_out, ",")) {
            *next_out = k_lexer_next(lexer, next_pp_kind_out);
            continue;
        }
        if (!k_token_is_punct(*next_out, ";")) return false;
        break;
    }

    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return true;
}
