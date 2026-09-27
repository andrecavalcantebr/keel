#include "engine/parser.h"

/* qual-c (keel-spec §2.2). All but `ref` are C words, so they are not
   idents for k_token_is_ident's purposes and need k_token_is_c_word_named. */
bool k_token_is_qual_c(KToken t) {
    return k_token_is_c_word_named(t, "const") ||
           k_token_is_c_word_named(t, "volatile") ||
           k_token_is_c_word_named(t, "restrict") ||
           k_token_is_c_word_named(t, "_Atomic") ||
           k_token_is_ident_named(t, "ref");
}

/* Skips a whole `suffix` body, from just after its opener through its
   matching `closer`, and returns the token after it — or the empty token
   if the source ended first. */
static KToken skip_suffix(KLexer *lexer, const char *closer, TKPpKind *pp) {
    const char *const terminators[1] = { closer };
    size_t which;
    KToken inside = k_lexer_next(lexer, pp);
    KToken end;
    k_scan_opaque_until(lexer, inside, terminators, 1, &which, &end, pp);
    if (end.len == 0) return end;
    return k_lexer_next(lexer, pp);
}

/* One level of `declarator`. Only the outermost level (depth 0) fills the
   shape fields of `*out`: they describe the declarator the caller asked
   for, not the one nested inside its parentheses. `out->name`, in
   contrast, is written by whichever level reaches the IDENT — the
   innermost one, which is the declared name (keel-spec §2.2). */
static bool scan(KLexer *lexer, KToken first, KDeclarator *out, int depth,
                 KToken *next_out, TKPpKind *pp) {
    /* A declarator nests only through '(' declarator ')', so this bound is
       reached only by pathological input; without it a long run of '('
       would recurse once per token. */
    if (depth > K_DECLARATOR_MAX_DEPTH) return false;

    KToken tok = first;
    int stars = 0;
    while (k_token_is_punct(tok, "*")) {
        stars++;
        tok = k_lexer_next(lexer, pp);
        while (k_token_is_qual_c(tok)) tok = k_lexer_next(lexer, pp);
    }
    if (depth == 0) out->pointer_depth = stars;

    /* direct-declarator ::= IDENT { suffix } | '(' declarator ')' { suffix }
       At the start of a direct-declarator a '(' can only open a nested
       declarator: a suffix's '(' is never first, it always follows one. */
    if (k_token_is_punct(tok, "(")) {
        if (depth == 0) out->parenthesized = true;
        KToken inner = k_lexer_next(lexer, pp);
        KToken after;
        if (!scan(lexer, inner, out, depth + 1, &after, pp)) return false;
        if (!k_token_is_punct(after, ")")) return false;
        tok = k_lexer_next(lexer, pp);
    } else if (k_token_is_ident(tok)) {
        out->name = tok;
        tok = k_lexer_next(lexer, pp);
    } else {
        /* No IDENT: an abstract-declarator, or malformed. Either way this
           production does not apply, and nothing was named. */
        return false;
    }

    for (;;) {
        if (k_token_is_punct(tok, "[")) {
            if (depth == 0) out->has_array_suffix = true;
            tok = skip_suffix(lexer, "]", pp);
        } else if (k_token_is_punct(tok, "(")) {
            if (depth == 0) out->has_function_suffix = true;
            tok = skip_suffix(lexer, ")", pp);
        } else {
            break;
        }
        if (tok.len == 0) break;   /* end of source inside the suffix */
    }

    *next_out = tok;
    return true;
}

bool k_scan_declarator(KLexer *lexer, KToken first, KDeclarator *out,
                        KToken *next_out, TKPpKind *next_pp_kind_out) {
    out->pointer_depth = 0;
    out->name = (keel_slice_char){ 0, NULL };
    out->parenthesized = false;
    out->has_array_suffix = false;
    out->has_function_suffix = false;
    return scan(lexer, first, out, 0, next_out, next_pp_kind_out);
}
