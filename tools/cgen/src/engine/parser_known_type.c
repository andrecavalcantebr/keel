#include "engine/parser.h"

/* qual-arg (keel-spec §2.2). All three are C words. */
static bool is_qual_arg(KToken t) {
    return k_token_is_c_word_named(t, "const") ||
           k_token_is_c_word_named(t, "volatile") ||
           k_token_is_c_word_named(t, "_Atomic");
}

static bool scan_known_type(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                             KSpecifier *out, int depth,
                             KToken *next_out, TKPpKind *pp);

/* One `argument`:
     argument    ::= { qual-arg } ( known-type | tagged-type | base-type
                                  | 'void' ) { qual-arg }
     tagged-type ::= ( 'struct' | 'union' | 'enum' ) qualified-name
     base-type   ::= 'char' | 'bool'
   An argument is several tokens far more often than one — `const char`,
   `struct Person`, and a whole nested `known-type` are all one argument.
   `*tok` is the argument's first token, already read, and is left on the
   token after the argument. The span returned runs from the first
   qualifier to the last, both sides included: keel-spec §2.2 accepts them
   "dos dois lados do tipo", and the canonical identity is a later
   contract's job, so what is recorded here is what was written. */
static bool scan_argument(KLexer *lexer, const KSymbolTable *symtab, int depth,
                           KToken *tok, keel_slice_char *out, TKPpKind *pp) {
    if (depth > K_SPEC_MAX_DEPTH) return false;
    if (tok->len == 0) return false;

    const char *from = tok->ptr;
    const char *to;

    while (is_qual_arg(*tok)) {
        *tok = k_lexer_next(lexer, pp);
        if (tok->len == 0) return false;
    }

    if (k_token_is_c_word_named(*tok, "struct") ||
        k_token_is_c_word_named(*tok, "union") ||
        k_token_is_c_word_named(*tok, "enum")) {
        *tok = k_lexer_next(lexer, pp);
        if (!k_token_is_ident(*tok)) return false;
        keel_slice_char name = k_scan_qualified_name(lexer, *tok, tok, pp);
        to = name.ptr + name.len;
    } else if (k_token_is_c_word_named(*tok, "char") ||
               k_token_is_c_word_named(*tok, "bool") ||
               k_token_is_c_word_named(*tok, "void")) {
        to = tok->ptr + tok->len;
        *tok = k_lexer_next(lexer, pp);
    } else if (k_token_is_ident(*tok)) {
        const KSymbol *sym = k_symtab_lookup(symtab, *tok);
        if (sym == NULL) return false;   /* not a keel argument */
        if (sym->kind == K_SYM_MODIFIER) {
            KSpecifier inner;
            if (!scan_known_type(lexer, *tok, symtab, &inner, depth + 1, tok, pp))
                return false;
            if (inner.kind != K_SPEC_MODIFIER) return false;
            to = inner.text.ptr + inner.text.len;
        } else if (sym->kind == K_SYM_TYPE || sym->kind == K_SYM_TAGS) {
            keel_slice_char name = k_scan_qualified_name(lexer, *tok, tok, pp);
            to = name.ptr + name.len;
        } else {
            return false;
        }
    } else {
        return false;
    }

    while (is_qual_arg(*tok)) {
        to = tok->ptr + tok->len;
        *tok = k_lexer_next(lexer, pp);
    }

    *out = (keel_slice_char){ (size_t)(to - from), (char *)from };
    return true;
}

static bool scan_known_type(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                             KSpecifier *out, int depth,
                             KToken *next_out, TKPpKind *pp) {
    if (depth > K_SPEC_MAX_DEPTH) return false;

    out->kind = K_SPEC_NONE;
    out->arg_count = 0;
    out->dim_count = 0;
    out->text = (keel_slice_char){ 0, NULL };

    const KSymbol *sym = k_symtab_lookup(symtab, first);
    if (sym == NULL) return true;   /* nothing consumed; *next_out untouched */

    if (sym->kind == K_SYM_TYPE || sym->kind == K_SYM_TAGS) {
        out->kind = K_SPEC_NAMED_TYPE;
        out->type_name = first;
        out->text = first;
        *next_out = k_lexer_next(lexer, pp);
        return true;
    }
    if (sym->kind != K_SYM_MODIFIER) return true;

    out->kind = K_SPEC_MODIFIER;
    out->modifier_name = first;
    const char *to = first.ptr + first.len;
    KToken tok = k_lexer_next(lexer, pp);

    /* modifier ::= qualified-name [ '(' dim-value { ',' dim-value } ')' ]
       dim-value ::= NUM | qualified-name */
    if (k_token_is_punct(tok, "(")) {
        for (;;) {
            tok = k_lexer_next(lexer, pp);
            if (tok.len == 0) return false;
            if (out->dim_count >= K_SPEC_MAX_DIMS) return false;
            if (k_token_is_number(tok)) {
                out->dims[out->dim_count++] = tok;
                tok = k_lexer_next(lexer, pp);
            } else if (k_token_is_ident(tok)) {
                out->dims[out->dim_count++] =
                    k_scan_qualified_name(lexer, tok, &tok, pp);
            } else {
                return false;
            }
            if (k_token_is_punct(tok, ",")) continue;
            if (k_token_is_punct(tok, ")")) break;
            return false;
        }
        to = tok.ptr + tok.len;             /* through the ')' */
        tok = k_lexer_next(lexer, pp);
    }

    /* known-type ::= modifier argument { argument } — the count is the
       declaring module's arity, from the symbol table, not a free
       repetition (keel-spec §2.2: "o número de argumentos de um
       modificador é determinado pelo módulo declarado"). */
    for (int i = 0; i < sym->arity; i++) {
        if (out->arg_count >= K_SPEC_MAX_ARGS) return false;
        keel_slice_char arg;
        if (!scan_argument(lexer, symtab, depth + 1, &tok, &arg, pp)) return false;
        out->args[out->arg_count++] = arg;
        to = arg.ptr + arg.len;
    }

    out->text = (keel_slice_char){ (size_t)(to - first.ptr), first.ptr };
    *next_out = tok;
    return true;
}

bool k_scan_known_type(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                        KSpecifier *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    return scan_known_type(lexer, first, symtab, out, 0, next_out, next_pp_kind_out);
}

bool k_scan_argument(KLexer *lexer, KToken first, const KSymbolTable *symtab,
                      keel_slice_char *out, KToken *next_out,
                      TKPpKind *next_pp_kind_out) {
    KToken tok = first;
    if (!scan_argument(lexer, symtab, 0, &tok, out, next_pp_kind_out)) return false;
    *next_out = tok;
    return true;
}
