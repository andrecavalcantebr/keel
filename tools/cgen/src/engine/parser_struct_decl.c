#include "engine/parser.h"

bool k_scan_struct_decl(KLexer *lexer, KToken struct_or_union_kw, KSymbolTable *symtab,
                         KStructDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    out->is_union = k_token_is_c_word_named(struct_or_union_kw, "union");
    out->name_count = 0;
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);

    if (k_token_is_ident(tok)) {
        out->tag_name = tok;
        tok = k_lexer_next(lexer, next_pp_kind_out);
    } else {
        out->tag_name = (keel_slice_char){0};
    }

    if (!k_token_is_punct(tok, "{")) return false;
    out->body = k_scan_braced_opaque(lexer, tok, &tok, next_pp_kind_out);

    bool ok = true;
    if (out->tag_name.len != 0)
        ok = k_symtab_insert(symtab, out->tag_name, K_SYM_TYPE, 0);

    /* [ declarator { ',' declarator } ] ';' — each declarator declares an
       object of this type, which is why they are variables and the tag
       is the type. */
    while (!k_token_is_punct(tok, ";")) {
        if (tok.len == 0) return false;
        if (out->name_count >= K_STRUCT_MAX_NAMES) return false;
        KDeclarator declarator;
        if (!k_scan_declarator(lexer, tok, &declarator, &tok, next_pp_kind_out))
            return false;
        out->names[out->name_count] = declarator.name;
        out->pointer_depth[out->name_count] = declarator.pointer_depth;
        out->name_count++;
        if (!k_symtab_insert(symtab, declarator.name, K_SYM_VARIABLE, 0)) ok = false;
        if (k_token_is_punct(tok, ",")) {
            tok = k_lexer_next(lexer, next_pp_kind_out);
            continue;
        }
        if (!k_token_is_punct(tok, ";")) return false;
    }

    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return ok;
}
