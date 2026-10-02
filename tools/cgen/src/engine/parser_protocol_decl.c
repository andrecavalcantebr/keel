/* decl-protocol (keel-spec §2.2, §5.1): the declaration of a named contract.
   A protocol emits no C; what the parser keeps is what a construction or a
   function over the protocol asks of a type — the verbs, their arity, where
   the receiver sits and the roles the prototype writes. */
#include <stdint.h>
#include "engine/parser.h"

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
