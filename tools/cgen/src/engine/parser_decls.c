#include <stdint.h>
#include "engine/parser.h"

/* ---- k_scan_decl_keel ---- */

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

/* ---- k_scan_decl_constexpr ---- */

bool k_scan_decl_constexpr(KLexer *lexer, KSymbolTable *symtab,
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

/* ---- k_scan_decl_typedef ---- */

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

/* ---- k_scan_decl_array ---- */

/* Written by the local model (task m2-parser-decl-array of the harness retired on
   2026-10-22), then
   finished by hand: the run blocked at 5 attempts, with the same three
   faults in every one — the stars looked for after the name instead of
   before it, the lexer never advanced past the name, and `*next_out`
   left standing on the ';'. The shape below, and the bracket-depth walk
   over the dimensions, are as delivered. */

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

/* ---- k_scan_struct_decl ---- */

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

/* ---- k_scan_tags_decl ---- */

bool k_scan_tags_decl(KLexer *lexer, KToken tags_kw, KSymbolTable *symtab,
                       KTagsDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    (void)tags_kw;
    out->name = k_lexer_next(lexer, next_pp_kind_out);
    out->item_count = 0;
    out->has_values = false;
    out->all_values = true;

    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_punct(tok, "[")) return false;
    tok = k_lexer_next(lexer, next_pp_kind_out);

    while (!k_token_is_punct(tok, "]")) {
        if (tok.len == 0 || !k_token_is_ident(tok)) return false;
        if (out->item_count >= K_TAGS_MAX_ITEMS) return false;
        size_t at = out->item_count++;
        out->items[at] = tok;
        out->values[at] = (keel_slice_char){ 0, NULL };

        tok = k_lexer_next(lexer, next_pp_kind_out);
        if (k_token_is_punct(tok, "=")) {
            /* tag-value ::= [ '-' ] ( NUM | qualified-name ) */
            out->has_values = true;
            tok = k_lexer_next(lexer, next_pp_kind_out);
            const char *from = tok.ptr;
            const char *to;
            if (k_token_is_punct(tok, "-")) tok = k_lexer_next(lexer, next_pp_kind_out);
            if (k_token_is_ident(tok)) {
                keel_slice_char name = k_scan_qualified_name(lexer, tok, &tok,
                                                             next_pp_kind_out);
                to = name.ptr + name.len;
            } else if (k_token_is_number(tok)) {
                to = tok.ptr + tok.len;
                tok = k_lexer_next(lexer, next_pp_kind_out);
            } else {
                return false;
            }
            out->values[at] = (keel_slice_char){ (size_t)(to - from), (char *)from };
        } else {
            out->all_values = false;
        }

        if (k_token_is_punct(tok, ",")) {
            tok = k_lexer_next(lexer, next_pp_kind_out);
            continue;
        }
        if (!k_token_is_punct(tok, "]")) return false;
    }

    tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_punct(tok, ";")) return false;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return k_symtab_insert(symtab, out->name, K_SYM_TAGS, 0);
}

/* ---- k_scan_modifier_decl ---- */

bool k_scan_modifier_decl(KLexer *lexer, int module_arity,
                          KSymbolTable *symtab, KModifierDecl *out,
                          KToken *next_out, TKPpKind *next_pp_kind_out) {
    out->name = k_lexer_next(lexer, next_pp_kind_out);
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    out->byref = false;

    if (k_token_is_ident_named(tok, "byref")) {
        out->byref = true;
        tok = k_lexer_next(lexer, next_pp_kind_out);
    }

    out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);
    return k_symtab_insert(symtab, out->name, K_SYM_MODIFIER, module_arity);
}

/* ---- k_scan_extent_decl ---- */

/* Written by the local model (task m2-parser-extent-decl of the harness retired on
   2026-10-22), then
   finished by hand: the run blocked at 5 attempts on three assertions,
   and only two of them were the code's fault (see below). */

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

/* ---- k_scan_decl_protocol ---- */

/* decl-protocol (keel-spec §2.2, §5.1): the declaration of a named contract.
   A protocol emits no C; what the parser keeps is what a construction or a
   function over the protocol asks of a type — the verbs, their arity, where
   the receiver sits and the roles the prototype writes. */

KRole k_token_role(KToken t) {
    if (!k_token_is_ident(t)) return K_ROLE_NONE;
    if (k_token_spelled(t, "parent")) return K_ROLE_PARENT;
    if (k_token_spelled(t, "child")) return K_ROLE_CHILD;
    if (k_token_spelled(t, "invalidates")) return K_ROLE_INVALIDATES;
    if (k_token_spelled(t, "consumes")) return K_ROLE_CONSUMES;
    return K_ROLE_NONE;
}

/* One prototype, from its first token through the ';'. */
static bool scan_verb(KLexer *lexer, KToken tok, keel_slice_char implementer,
                      KProtocolVerb *v, TKPpKind *pp) {
    *v = (KProtocolVerb){ .receiver = SIZE_MAX };
    if (k_token_role(tok) == K_ROLE_CHILD) {
        v->result_role = K_ROLE_CHILD;
        tok = k_lexer_next(lexer, pp);
    }
    KToken previous = { 0 };
    while (tok.len != 0 && !k_token_is_punct(tok, "(")) {
        if (k_token_is_punct(tok, ";") || k_token_is_punct(tok, "}")) return false;
        if (k_token_is_ident(previous)) v->returns = previous;
        previous = tok;
        tok = k_lexer_next(lexer, pp);
    }
    if (tok.len == 0 || !k_token_is_ident(previous)) return false;
    v->name = previous;

    /* the parameters: a role may lead each one, and the one whose first
       type token is the implementer is the receiver */
    tok = k_lexer_next(lexer, pp);
    if (k_token_is_punct(tok, ")")) {
        tok = k_lexer_next(lexer, pp);
    } else {
        size_t depth = 0;
        bool lead = true;
        for (;;) {
            if (tok.len == 0) return false;
            if (lead) {
                if (v->arity >= K_PROTOCOL_MAX_PARAMS) return false;
                KRole role = k_token_role(tok);
                v->roles[v->arity] = role;
                if (role != K_ROLE_NONE) tok = k_lexer_next(lexer, pp);
                if (k_token_spelled(tok, "void")) {
                    KToken after = k_lexer_next(lexer, pp);
                    if (k_token_is_punct(after, ")") && v->arity == 0) { tok = k_lexer_next(lexer, pp); break; }
                    tok = after;
                    v->arity++;
                    lead = false;
                    continue;
                }
                if (implementer.len && k_symtab_same_name(tok, implementer) && v->receiver == SIZE_MAX)
                    v->receiver = v->arity;
                v->arity++;
                lead = false;
            }
            if (k_token_is_punct(tok, "(") || k_token_is_punct(tok, "[")) depth++;
            else if (depth && (k_token_is_punct(tok, ")") || k_token_is_punct(tok, "]"))) depth--;
            else if (!depth && k_token_is_punct(tok, ",")) lead = true;
            else if (!depth && k_token_is_punct(tok, ")")) { tok = k_lexer_next(lexer, pp); break; }
            tok = k_lexer_next(lexer, pp);
        }
    }
    return k_token_is_punct(tok, ";");
}

bool k_scan_decl_protocol(KLexer *lexer, KToken protocol_kw, KSymbolTable *symtab,
                          KProtocolDecl *out, KToken *next_out, TKPpKind *next_pp_kind_out) {
    (void)protocol_kw;
    *out = (KProtocolDecl){ 0 };
    out->name = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_ident(out->name)) return false;
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);

    if (k_token_is_punct(tok, "[")) {
        tok = k_lexer_next(lexer, next_pp_kind_out);
        while (!k_token_is_punct(tok, "]")) {
            if (!k_token_is_ident(tok) || out->component_count >= K_PROTOCOL_MAX_ITEMS) return false;
            out->components[out->component_count++] =
                k_scan_qualified_name(lexer, tok, &tok, next_pp_kind_out);
            if (k_token_is_punct(tok, ",")) tok = k_lexer_next(lexer, next_pp_kind_out);
            else if (!k_token_is_punct(tok, "]")) return false;
        }
        tok = k_lexer_next(lexer, next_pp_kind_out);
        if (!k_token_is_punct(tok, ";")) return false;
        *next_out = k_lexer_next(lexer, next_pp_kind_out);
        return k_symtab_insert(symtab, out->name, K_SYM_PROTOCOL, 0);
    }

    if (!k_token_spelled(tok, "type")) return false;
    out->implementer = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_ident(out->implementer)) return false;
    tok = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_punct(tok, "{")) return false;
    tok = k_lexer_next(lexer, next_pp_kind_out);
    while (!k_token_is_punct(tok, "}")) {
        if (tok.len == 0) return false;
        if (k_token_spelled(tok, "type")) {
            KToken name = k_lexer_next(lexer, next_pp_kind_out);
            if (!k_token_is_ident(name) || out->assoc_count >= K_PROTOCOL_MAX_ITEMS) return false;
            out->assoc[out->assoc_count++] = name;
            tok = k_lexer_next(lexer, next_pp_kind_out);
            if (!k_token_is_punct(tok, ";")) return false;
        } else {
            if (out->verb_count >= K_PROTOCOL_MAX_ITEMS) return false;
            if (!scan_verb(lexer, tok, out->implementer, &out->verbs[out->verb_count++], next_pp_kind_out))
                return false;
        }
        tok = k_lexer_next(lexer, next_pp_kind_out);
    }
    tok = k_lexer_next(lexer, next_pp_kind_out);
    if (k_token_is_punct(tok, ";")) tok = k_lexer_next(lexer, next_pp_kind_out);
    *next_out = tok;
    return k_symtab_insert(symtab, out->name, K_SYM_PROTOCOL, 0);
}

/* ---- k_scan_extern_c ---- */

void k_scan_extern_c(KLexer *lexer, KExternCDecl *out,
                     KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);
    out->has_type_h = false;

    if (k_token_is_punct(tok, "[")) {
        out->has_type_h = true;
        k_lexer_next(lexer, next_pp_kind_out); // type_h
        k_lexer_next(lexer, next_pp_kind_out); // ]
        tok = k_lexer_next(lexer, next_pp_kind_out);
    }

    out->body = k_scan_braced_opaque(lexer, tok, next_out, next_pp_kind_out);
}
