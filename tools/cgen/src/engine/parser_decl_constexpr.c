#include "engine/parser.h"

bool k_scan_decl_constexpr(KLexer *lexer, KToken constexpr_kw, KSymbolTable *symtab,
                            KConstexprDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    bool has_prev = false;
    KToken prev = {0};
    KToken cur = k_lexer_next(lexer, next_pp_kind_out);
    int depth = 0;

    while (true) {
        if (!cur.len) return false;
        if (depth == 0 && k_token_is_punct(cur, "=")) {
            if (!has_prev || !k_token_is_ident(prev)) {
                return false;
            }
            out->has_name = true;
            out->name = prev;
            if (!k_symtab_insert(symtab, prev, K_SYM_CONSTANT, 0)) return false;
            KToken init_first = k_lexer_next(lexer, next_pp_kind_out);
            static const char *terms[] = { ";" };
            size_t idx;
            k_scan_opaque_until(lexer, init_first, terms, 1, &idx, next_out, next_pp_kind_out);
            if (!k_token_is_punct(*next_out, ";")) return false;
            *next_out = k_lexer_next(lexer, next_pp_kind_out);
            return true;
        } else if (depth == 0 && k_token_is_punct(cur, ";")) {
            out->has_name = false;
            out->name = (keel_slice_char){0};
            *next_out = k_lexer_next(lexer, next_pp_kind_out);
            return true;
        } else {
            if (cur.ptr[0] == '(' || cur.ptr[0] == '[' || cur.ptr[0] == '{') {
                depth++;
            } else if (cur.ptr[0] == ')' || cur.ptr[0] == ']' || cur.ptr[0] == '}') {
                depth--;
            }
            prev = cur;
            has_prev = true;
            cur = k_lexer_next(lexer, next_pp_kind_out);
        }
    }
}