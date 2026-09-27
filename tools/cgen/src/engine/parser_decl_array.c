/* Written by the local model (tools/harness, m2-parser-decl-array), then
   finished by hand: the run blocked at 5 attempts, with the same three
   faults in every one — the stars looked for after the name instead of
   before it, the lexer never advanced past the name, and `*next_out`
   left standing on the ';'. The shape below, and the bracket-depth walk
   over the dimensions, are as delivered. */
#include "engine/parser.h"

bool k_scan_decl_array(KLexer *lexer, KToken array_kw, KSymbolTable *symtab,
                        KArrayDecl *out, KToken *next_out,
                        TKPpKind *next_pp_kind_out) {
    (void)array_kw;
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_scan_argument(lexer, tok, symtab, &out->element, &tok, next_pp_kind_out))
        return false;

    out->name_count = 0;

    for (;;) {
        if (out->name_count >= K_ARRAY_MAX_NAMES) return false;
        size_t at = out->name_count;

        /* decl-array-1 ::= { '*' } IDENT dimensions [ '=' <opaque> ]
           — every star comes BEFORE the name. */
        out->pointer_depth[at] = 0;
        while (k_token_is_punct(tok, "*")) {
            out->pointer_depth[at]++;
            tok = k_lexer_next(lexer, next_pp_kind_out);
        }
        if (!k_token_is_ident(tok)) return false;
        out->names[at] = tok;
        tok = k_lexer_next(lexer, next_pp_kind_out);

        /* `dimensions` is required, and its two spellings — `[2,3,4]`
           and `[2][3][4]` — are one run of bracket groups either way, so
           one depth counter walks both, and `[a[b]]` with it. */
        if (!k_token_is_punct(tok, "[")) return false;
        const char *dims_from = tok.ptr;
        const char *dims_to = NULL;
        size_t depth = 0;
        for (;;) {
            if (k_token_is_punct(tok, "[")) {
                depth++;
            } else if (k_token_is_punct(tok, "]")) {
                depth--;
                if (depth == 0) {
                    dims_to = tok.ptr + tok.len;
                    tok = k_lexer_next(lexer, next_pp_kind_out);
                    if (!k_token_is_punct(tok, "[")) break;
                    continue;
                }
            }
            tok = k_lexer_next(lexer, next_pp_kind_out);
            if (tok.len == 0) return false;
        }
        out->dims[at] = (keel_slice_char){ (size_t)(dims_to - dims_from),
                                           (char *)dims_from };

        if (!k_symtab_insert(symtab, out->names[at], K_SYM_VARIABLE, 0)) return false;
        out->name_count++;

        if (k_token_is_punct(tok, "=")) {
            static const char *const terms[] = { ",", ";" };
            size_t which;
            k_scan_opaque_until(lexer, tok, terms, 2, &which, &tok, next_pp_kind_out);
        }
        if (k_token_is_punct(tok, ",")) {
            tok = k_lexer_next(lexer, next_pp_kind_out);
            continue;
        }
        if (!k_token_is_punct(tok, ";")) return false;
        *next_out = k_lexer_next(lexer, next_pp_kind_out);
        return true;
    }
}
