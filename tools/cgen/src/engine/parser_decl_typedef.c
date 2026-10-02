#include "engine/parser.h"

/* decl-typedef (keel-spec §2.2):

     decl-typedef ::= 'typedef' ( specifier | struct-spec |
                                  <opaque-no-parens> )
                      ( declarator [ 'byref' ] | declarator { ',' declarator } ) ';'

   The specifier is shared and opaque to this production: it is only
   walked past, and `k_find_declarator_start` says where it ends. Each
   declarator's own name is what gets registered, so
   `typedef int a[4], *b;` registers both `a` and `b`. */
bool k_scan_decl_typedef(KLexer *lexer, KToken typedef_kw, KSymbolTable *symtab,
                          KTypedefDecl *out, KToken *next_out,
                          TKPpKind *next_pp_kind_out) {
    (void)typedef_kw;
    out->name_count = 0;

    KToken tok = k_lexer_next(lexer, next_pp_kind_out);

    KToken start = k_find_declarator_start(lexer, tok, NULL);
    if (start.len == 0) return false;

    /* The specifier is opaque to this production, so it is only walked
       past — by the caller's lexer, not the scout's, so its diagnostics
       are the ones that reach the sink. */
    while (tok.len != 0 && tok.ptr < start.ptr) tok = k_lexer_next(lexer, next_pp_kind_out);
    if (tok.len == 0) return false;

    for (;;) {
        KDeclarator declarator;
        KToken after;
        if (!k_scan_declarator(lexer, tok, &declarator, &after, next_pp_kind_out))
            return false;
        if (out->name_count >= K_TYPEDEF_MAX_NAMES) return false;
        out->names[out->name_count++] = declarator.name;
        if (!k_symtab_insert(symtab, declarator.name, K_SYM_TYPE, 0)) return false;

        if (k_token_is_punct(after, ",")) {
            /* the specifier is shared, so the next declarator starts at
               the very next token — no second lookahead */
            tok = k_lexer_next(lexer, next_pp_kind_out);
            continue;
        }
        if (out->name_count == 1 && k_token_spelled(after, "byref")) {
            after = k_lexer_next(lexer, next_pp_kind_out);
            if (!k_token_is_punct(after, ";")) return false;
        }
        if (k_token_is_punct(after, ";")) {
            *next_out = k_lexer_next(lexer, next_pp_kind_out);
            return true;
        }
        return false;
    }
}
