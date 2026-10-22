#include "engine/parser.h"
#include "engine/names.h"

/* ---- k_scan_declarator ---- */

/* qual-c (keel-spec §2.2). All but `ref` are C words, so they are not
   idents for k_token_is_ident's purposes and need k_token_is_c_word_named. */
static bool k_token_is_qual_c(KToken t) {
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

/* Whether a `declarator` starting at `from` parses, names something, and
   stops on a token that can follow one in a declaration. */
/* `byref ;` after a typedef's single declarator (keel-spec §2.2): `t` is the
   token after the declarator, and `lexer` stands right after it. */
static bool byref_tail(const KLexer *lexer, KToken t) {
    if (!k_token_spelled(t, "byref")) return false;
    KLexer peek = *lexer;
    TKPpKind pp;
    return k_token_is_punct(k_lexer_next(&peek, &pp), ";");
}

static bool parses_as_declarator(const KLexer *lexer, KToken from) {
    keel_slice_char rest = {
        (size_t)((lexer->source.ptr + lexer->source.len) - from.ptr), from.ptr
    };
    KLexer scout;
    TKPpKind pp;
    k_lexer_init(&scout, rest, NULL);

    KDeclarator declarator;
    KToken next;
    if (!k_scan_declarator(&scout, k_lexer_next(&scout, &pp), &declarator, &next, &pp))
        return false;
    if (declarator.name.len == 0) return false;
    return k_token_is_punct(next, ";") || k_token_is_punct(next, ",") ||
           k_token_is_punct(next, "=") || k_token_is_punct(next, "{") ||
           byref_tail(&scout, next);
}

KToken k_find_declarator_start(const KLexer *lexer, KToken first, const char *limit) {
    keel_slice_char rest = {
        (size_t)((lexer->source.ptr + lexer->source.len) - first.ptr), first.ptr
    };
    KLexer scout;
    TKPpKind pp;
    k_lexer_init(&scout, rest, NULL);

    KToken tok = k_lexer_next(&scout, &pp);
    KToken best = { 0, NULL };
    int depth = 0;

    while (tok.len != 0 && (limit == NULL || tok.ptr < limit)) {
        if (depth == 0) {
            /* Nothing after these can still be the declarator: the ';'
               ends the declaration, the ',' hands the rest to the next
               declarator, and the '=' opens an initializer — whose own
               identifiers would otherwise look like later candidates
               (`int x = y;` must name x, not y). */
            if (k_token_is_punct(tok, ";") || k_token_is_punct(tok, ",") ||
                k_token_is_punct(tok, "=") || byref_tail(&scout, tok)) break;
            /* Latest wins: the specifier is maximal, so of two starts
               that both parse, the declarator is the later one. */
            if (parses_as_declarator(lexer, tok)) best = tok;
        }
        if (k_token_is_punct(tok, "(") || k_token_is_punct(tok, "[") ||
            k_token_is_punct(tok, "{")) depth++;
        else if (k_token_is_punct(tok, ")") || k_token_is_punct(tok, "]") ||
                 k_token_is_punct(tok, "}")) depth--;
        tok = k_lexer_next(&scout, &pp);
    }
    return best;
}

/* ---- k_scan_known_type ---- */

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
        KLexer look=*lexer; KToken after; TKPpKind look_pp;
        keel_slice_char qualified=k_scan_qualified_name(&look,*tok,&after,&look_pp);
        const KSymbol *sym = k_symbol_resolve(symtab, qualified);
        const char *primitive=k_primitive_name(qualified);
        if (!sym && primitive) { *lexer=look; *tok=after; *pp=look_pp;
            to=qualified.ptr+qualified.len; goto trailing; }
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

trailing:
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

    KLexer look=*lexer; KToken after; TKPpKind look_pp;
    keel_slice_char qualified=k_scan_qualified_name(&look,first,&after,&look_pp);
    const KSymbol *sym = k_symbol_resolve(symtab, qualified);
    if (sym == NULL) return true;   /* nothing consumed; *next_out untouched */

    if (sym->kind == K_SYM_TYPE || sym->kind == K_SYM_TAGS) {
        out->kind = K_SPEC_NAMED_TYPE;
        out->type_name = qualified;
        out->text = qualified;
        *lexer=look; *next_out=after; *pp=look_pp;
        return true;
    }
    if (sym->kind != K_SYM_MODIFIER) return true;

    out->kind = K_SPEC_MODIFIER;
    out->modifier_name = qualified;
    const char *to = qualified.ptr + qualified.len;
    *lexer=look; *pp=look_pp;
    KToken tok = after;

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
    for (int i = 0; i < sym->arity - sym->dim_arity; i++) {
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
