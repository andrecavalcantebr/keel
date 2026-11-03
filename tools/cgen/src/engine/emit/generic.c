/* engine/emit/generic.c — the partition of a generic module (emit_internal.h;
 * backend §4.4.1, §5.19; spec §4.3): what the module's own headers write is its
 * share, and an instance writes what names its binders or its modifier. No I/O,
 * no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/names.h"
#include "engine/symtab.h"

/* ---- generic modules (backend §4.4.1) ----
 * A module with binders (`module coll type T;`) writes headers only for what
 * mentions none of its binders and none of its modifiers: that is the
 * module's share, and the rest belongs to the instances (T21). The partition is
 * lexical. A node is out when it is a modifier, or when one of its identifier
 * tokens spells a binder or a modifier name. A module without binders has
 * nothing out. */

static bool same_text(KToken a, KToken b) { return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0; }

bool k_em_module_is_generic(const KAst *ast) {
    const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    return m->dim_first != m->dim_end || m->tags_first != m->tags_end || m->type_first != m->type_end;
}

/* whether `t` spells one of the module's binders or one of its modifiers */
static bool names_binder_or_modifier(const KAst *ast, KToken t) {
    const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    size_t first[3] = { m->type_first, m->dim_first, m->tags_first };
    size_t end[3] = { m->type_end, m->dim_end, m->tags_end };
    for (int r = 0; r < 3; r++)
        for (size_t j = first[r]; j < end[r]; j++) {
            KToken x = k_em_tok(ast, j);
            if (k_token_is_ident(x) && same_text(x, t)) return true;
        }
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind == K_AST_MODIFIER && n->name_first < n->name_end && same_text(k_em_tok(ast, n->name_first), t))
            return true;
    }
    return false;
}

/* whether the declaration `n` of a generic module belongs to the instance */
/* a function with a parameter typed by a protocol (backend §5.19): one instance per concrete type */
bool k_em_over_protocol(const KAst *ast, const KAstNode *n);
#define over_protocol k_em_over_protocol
bool k_em_over_protocol(const KAst *ast, const KAstNode *n) {
    if (n->kind != K_AST_FUNCTION || !ast->symbols || n->body_first == (size_t)-1) return false;
    for (size_t i = n->name_first + 1; i < n->body_first; i++) {
        KToken t = k_em_tok(ast, i);
        if (!k_token_is_ident(t)) continue;
        const KSymbol *s = k_symbol_resolve(ast->symbols, (keel_slice_char){ t.len, (char *)t.ptr });
        if (s && s->kind == K_SYM_PROTOCOL) return true;
    }
    return false;
}

bool k_em_generic_out(const KAst *ast, const KAstNode *n) {
    if (over_protocol(ast, n)) return true;
    if (n->kind == K_AST_MODULE || n->kind == K_AST_IMPORT || !k_em_module_is_generic(ast)) return false;
    if (n->kind == K_AST_MODIFIER) return true;
    for (size_t i = n->first; i < n->end; i++) {
        KToken t = k_em_tok(ast, i);
        if (k_token_is_ident(t) && names_binder_or_modifier(ast, t)) return true;
    }
    return false;
}

static bool same_slice(KToken t, keel_slice_char s) { return t.len == s.len && memcmp(t.ptr, s.ptr, s.len) == 0; }

bool k_em_other_modifier(const Emit *e, const KAstNode *n) {
    if (!e->inst || !e->inst->modifier.len || e->inst->proto_fn.len) return false;
    const KAst *ast = e->ast;
    if (n->kind == K_AST_MODIFIER) return n->name_first < n->name_end && !same_slice(k_em_tok(ast, n->name_first), e->inst->modifier);
    if (n->kind != K_AST_FUNCTION || n->name_first >= n->name_end) return false;
    size_t end = n->body_first != (size_t)-1 ? n->body_first : n->end;
    for (size_t i = n->name_first + 1; i < end; i++) {
        KToken t = k_em_tok(ast, i);
        if (!k_token_is_ident(t) || same_slice(t, e->inst->modifier)) continue;
        for (size_t k = 0; k < ast->nodes.len; k++) {
            const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            if (m->kind == K_AST_MODIFIER && m->name_first < m->name_end && same_text(k_em_tok(ast, m->name_first), t)) return true;
        }
    }
    return false;
}

/* whether this file leaves `n` out: a module writes its share (k_em_generic_out
 * false); an instance (e->inst), the functions that belong to it */
bool k_em_skip(const Emit *e, const KAstNode *n);
#define skip k_em_skip
bool k_em_skip(const Emit *e, const KAstNode *n) {
    bool out = k_em_generic_out(e->ast, n);
    /* a function over a protocol is neither: one instance per concrete type (backend §5.19) */
    if (!e->inst) return out;
    if (e->inst->proto_fn.len) {   /* the functions of that name over the protocol */
        if (n->kind != K_AST_FUNCTION || !over_protocol(e->ast, n)) return true;
        KToken t = k_em_tok(e->ast, n->name_first);
        return t.len != e->inst->proto_fn.len || memcmp(t.ptr, e->inst->proto_fn.ptr, t.len) != 0;
    }
    if (!(out && n->kind == K_AST_FUNCTION && !over_protocol(e->ast, n))) return true;
    if (k_em_other_modifier(e, n)) return true;
    /* a verb the instance loses (spec §4.3, rules 11 to 13): the parser's list */
    KToken name = k_em_tok(e->ast, n->name_first);
    size_t arity = k_em_param_count(e->ast, n);
    for (size_t k = 0; k < e->inst->unavailable_count; k++) {
        const KUnavailable *u = &e->inst->unavailable[k];
        if (u->arity == arity && u->verb.len == name.len && memcmp(u->verb.ptr, name.ptr, name.len) == 0) return true;
    }
    return false;
}

/* whether a function of the instance names a declaration of the generic's
 * .type.h (a type, a constant, a set): it reaches it through that header
 * (backend §4.4.1, rule 4) */
bool k_em_uses_share(const Emit *e) {
    const KAst *ast = e->ast;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (skip(e, n)) continue;
        for (size_t j = n->first; j < n->end; j++) {
            KToken t = k_em_tok(ast, j);
            if (!k_token_is_ident(t)) continue;
            const KAstNode *d = k_em_declared_at(e, j);
            if (d && !k_em_generic_out(ast, d) && (d->kind == K_AST_TYPE || d->kind == K_AST_CONSTEXPR || d->kind == K_AST_TAGS))
                return true;
        }
    }
    return false;
}
