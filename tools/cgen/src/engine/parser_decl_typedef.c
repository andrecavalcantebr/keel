#include "engine/parser.h"

/* Where the first `declarator` of a decl-typedef begins — the one thing
   the production does not mark with a token of its own:

     decl-typedef ::= 'typedef' ( specifier | struct-spec |
                                  <opaque-no-parens> )
                      declarator { ',' declarator } ';'

   Reads ahead over a throwaway lexer (diagnostics off: the caller's own
   lexer walks the same bytes right after, and it carries the sink, so
   every byte is still reported exactly once). `first` is the first token
   after 'typedef'.

   The split is the one the grammar allows, and there is only one: the
   specifier cannot contain a '(' at its outer level, so a top-level '('
   can only be the declarator's own `'(' declarator ')'` — that is what
   makes `typedef int (*fp)(void);` name `fp` and not `void`. With no such
   '(', the declarator is the trailing `{ '*' { qual-c } } IDENT`, so the
   answer is the last name-or-star run before the first top-level '[',
   ',' or ';' — which is why `typedef int Vec[TAM];` names `Vec` and not
   `TAM`. A struct-spec's braces are skipped by the same depth counter
   that keeps the specifier's own tokens out of the way. */
static KToken declarator_start(const KLexer *lexer, KToken first) {
    keel_slice_char rest = {
        (size_t)((lexer->source.ptr + lexer->source.len) - first.ptr), first.ptr
    };
    KLexer scout;
    TKPpKind pp;
    k_lexer_init(&scout, rest, NULL);

    KToken tok = k_lexer_next(&scout, &pp);
    KToken candidate = tok;
    int depth = 0;
    bool in_stars = false;

    while (tok.len != 0) {
        if (depth == 0) {
            if (k_token_is_punct(tok, "(")) return tok;
            if (k_token_is_punct(tok, "[") || k_token_is_punct(tok, ",") ||
                k_token_is_punct(tok, ";")) break;
            if (k_token_is_punct(tok, "*")) {
                if (!in_stars) { candidate = tok; in_stars = true; }
            } else if (in_stars) {
                /* the run ends at its name; the run's leading '*' stays
                   the candidate, because the declarator starts there */
                if (!k_token_is_qual_c(tok)) in_stars = false;
            } else if (k_token_is_ident(tok) || k_token_is_c_word(tok)) {
                candidate = tok;
            }
        }
        if (k_token_is_punct(tok, "(") || k_token_is_punct(tok, "[") ||
            k_token_is_punct(tok, "{")) depth++;
        else if (k_token_is_punct(tok, ")") || k_token_is_punct(tok, "]") ||
                 k_token_is_punct(tok, "}")) depth--;
        tok = k_lexer_next(&scout, &pp);
    }
    return candidate;
}

bool k_scan_decl_typedef(KLexer *lexer, KToken typedef_kw, KSymbolTable *symtab,
                          KTypedefDecl *out, KToken *next_out,
                          TKPpKind *next_pp_kind_out) {
    (void)typedef_kw;
    out->name_count = 0;

    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    KToken start = declarator_start(lexer, tok);

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
        if (k_token_is_punct(after, ";")) {
            *next_out = k_lexer_next(lexer, next_pp_kind_out);
            return true;
        }
        return false;
    }
}
