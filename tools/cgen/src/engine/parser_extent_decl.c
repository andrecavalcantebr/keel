/* Written by the local model (tools/harness, m2-parser-extent-decl), then
   finished by hand: the run blocked at 5 attempts on three assertions,
   and only two of them were the code's fault (see below). */
#include "engine/parser.h"

/* extent-dim ::= '[' IDENT ',' ( qualified-name | NUM ) ']'
   `open` is the already-read '['. Leaves *tok on the token after the ']'. */
static bool scan_dim(KLexer *lexer, KExtentDecl *out, KToken *tok, TKPpKind *pp) {
    if (out->dim_count >= K_EXTENT_MAX_DIMS) return false;

    *tok = k_lexer_next(lexer, pp);
    if (!k_token_is_ident(*tok)) return false;
    out->dim_names[out->dim_count] = *tok;

    *tok = k_lexer_next(lexer, pp);
    if (!k_token_is_punct(*tok, ",")) return false;

    *tok = k_lexer_next(lexer, pp);
    if (k_token_is_number(*tok)) {
        out->dim_caps[out->dim_count] = *tok;
        *tok = k_lexer_next(lexer, pp);
    } else if (k_token_is_ident(*tok)) {
        /* a capacity may be a qualified name, not just one IDENT */
        out->dim_caps[out->dim_count] = k_scan_qualified_name(lexer, *tok, tok, pp);
    } else {
        return false;
    }

    if (!k_token_is_punct(*tok, "]")) return false;
    out->dim_count++;
    *tok = k_lexer_next(lexer, pp);
    return true;
}

bool k_scan_extent_decl(KLexer *lexer, KToken extent_kw, KSymbolTable *symtab,
                         KExtentDecl *out, KToken *next_out,
                         TKPpKind *next_pp_kind_out) {
    (void)extent_kw;
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_c_word_named(tok, "struct")) return false;

    tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_ident(tok)) return false;
    out->name = tok;

    /* the production requires at least one dimension */
    out->dim_count = 0;
    tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_punct(tok, "[")) return false;
    while (k_token_is_punct(tok, "[")) {
        if (!scan_dim(lexer, out, &tok, next_pp_kind_out)) return false;
    }

    if (!k_token_is_punct(tok, "{")) return false;
    out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);

    if (!k_token_is_punct(*next_out, ";")) return false;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);

    return k_symtab_insert(symtab, out->name, K_SYM_TYPE, 0);
}
