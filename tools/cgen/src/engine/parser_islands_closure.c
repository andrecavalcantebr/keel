/* Pass 3: the closure of the instances, provenance and invalidation, and the instance of each verb (spec §4.3). Split from parser_islands.c by T3; the code is unchanged. */

#include "engine/parser_islands_internal.h"

void isl_roles_at_call(Ctx *c, const KAst *home, const Sig *sig, const Range *args, size_t argc, size_t callee,
                          keel_slice_char verb);
int isl_local_of(const Ctx *c, Range r);
void isl_produce(Ctx *c, int p, const int *parents, int np);
bool isl_verb_available(Ctx *c, const KAst *home, const KAstNode *fn, int t, const char **cause);
/* ---- the closure of the instances (stage 4i) --------------------------- */

static bool qualified_const(const char *symbol) { return !strncmp(symbol, "const_", 6); }

/* the type written for parameter `p`, without its name, role and stars */
Range isl_param_type(const KAst *home, const Param *p) {
    Range r = { p->first, p->end ? p->end - 1 : p->first };
    if (p->role != K_ROLE_NONE) r.first++;
    while (r.end > r.first && isl_punct(home, r.end - 1, "*")) r.end--;
    return r;
}

/* the type written for `name` where it is declared in `fn` — a parameter, or a
   declaration in the body before token `before` */
static bool declared_type(const KAst *home, const KAstNode *fn, KToken name, size_t before, keel_slice_char *out) {
    Param p[K_PARAMS_MAX];
    size_t n = isl_params_of(home, fn, p, K_PARAMS_MAX);
    for (size_t k = 0; n != SIZE_MAX && k < n; k++)
        if (p[k].end > p[k].first && k_symtab_same_name(isl_tok(home, p[k].end - 1), name)) {
            Range r = isl_param_type(home, &p[k]);
            if (r.end <= r.first) return false;
            *out = isl_range_text(home, r);
            return true;
        }
    for (size_t j = fn->body_first; j < before && j < fn->body_end; j++) {
        if (!k_symtab_same_name(isl_tok(home, j), name) || !(isl_punct(home, j + 1, "=") || isl_punct(home, j + 1, ";"))) continue;
        size_t st = j;
        while (st > fn->body_first && !isl_punct(home, st - 1, ";") && !isl_punct(home, st - 1, "{") && !isl_punct(home, st - 1, "}"))
            st--;
        size_t e = j;
        while (e > st && isl_punct(home, e - 1, "*")) e--;
        if (e <= st) return false;
        *out = isl_range_text(home, (Range){ st, e });
        return true;
    }
    return false;
}

/* the fields of the modifiers of `home` whose type is binder `b`: by value
   (`T value`) or as the pointee (`T *ptr`) */
static bool binder_field(const KAst *home, keel_slice_char b, keel_slice_char field, bool *pointer) {
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *m = isl_node_at(home, i);
        if (m->kind != K_AST_MODIFIER || m->body_first == SIZE_MAX) continue;
        for (size_t j = m->body_first; j + 2 < m->body_end; j++) {
            if (!k_symtab_same_name(isl_tok(home, j), b)) continue;
            if (j > m->body_first && !isl_punct(home, j - 1, ";") && !isl_punct(home, j - 1, "{")) continue;
            if (isl_ident(home, j + 1) && k_symtab_same_name(isl_tok(home, j + 1), field)) { *pointer = false; return true; }
            if (isl_punct(home, j + 1, "*") && isl_ident(home, j + 2) && k_symtab_same_name(isl_tok(home, j + 2), field)) {
                *pointer = true;
                return true;
            }
        }
    }
    return false;
}

bool isl_verb_available(Ctx *c, const KAst *home, const KAstNode *fn, int t, const char **cause);

/* the arguments of the call whose '(' is `open`, up to `close` */
size_t isl_call_args(const KAst *a, size_t open, size_t close, Range *first) {
    size_t n = 0, depth = 0, st = open + 1;
    if (close == open + 1) return 0;
    for (size_t j = open + 1; j <= close; j++) {
        if (j < close && (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{"))) depth++;
        else if (j < close && (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}"))) depth--;
        else if (j == close || (!depth && isl_punct(a, j, ","))) {
            if (n == 0) *first = (Range){ st, j };
            n++;
            st = j + 1;
        }
    }
    return n;
}

/* Spec §4.3, rules 11 to 13: a verb of an instance whose argument is `void`
   loses the reads and writes of the omitted `T value` and the parameters of
   type `T` by value; one whose argument is `const` loses the writes to the
   constant value; and a verb that calls a lost verb is lost with it. */
static bool compute_surface(Ctx *c, const KAst *home, const KAstNode *fn, int t, Text *why) {
    const KType *k = isl_kt(c, t);
    keel_slice_char binders[8];
    size_t nb = isl_binders_of(home, binders);
    bool is_void[8] = {0}, is_const[8] = {0};
    for (size_t b = 0; b < nb && b < (size_t)k->argc; b++) {
        const char *sym = isl_kt_symbol(c, k->arg[b]);
        is_void[b] = !strcmp(sym, "void");
        is_const[b] = qualified_const(sym);
    }
    Param p[K_PARAMS_MAX];
    size_t np = isl_params_of(home, fn, p, K_PARAMS_MAX);
    for (size_t q = 0; np != SIZE_MAX && q < np; q++) {
        Range r = isl_param_type(home, &p[q]);
        if (r.end != r.first + 1 || p[q].pointer) continue;
        for (size_t b = 0; b < nb; b++)
            if (is_void[b] && k_symtab_same_name(isl_tok(home, r.first), binders[b])) {
                isl_put_str(why, "void-param ");
                isl_put(why, isl_tok(home, p[q].end - 1).ptr, isl_tok(home, p[q].end - 1).len);
                return false;
            }
    }
    if (fn->body_first == SIZE_MAX) return true;
    for (size_t j = fn->body_first; j < fn->body_end; j++) {
        /* a field of the instance: `x->value`, `x.ptr[i] = v` */
        if (isl_ident(home, j) && j > fn->body_first && (isl_punct(home, j - 1, "->") || isl_punct(home, j - 1, ".")) &&
            !isl_punct(home, j + 1, "(")) {
            for (size_t b = 0; b < nb; b++) {
                bool pointer;
                if ((!is_void[b] && !is_const[b]) || !binder_field(home, binders[b], isl_tok(home, j), &pointer)) continue;
                if (is_void[b] && !pointer) {
                    isl_put_str(why, "void-field ");
                    isl_put(why, isl_tok(home, j).ptr, isl_tok(home, j).len);
                    return false;
                }
                if (!is_const[b]) continue;
                size_t after = j + 1;
                if (pointer) {
                    if (!isl_punct(home, after, "[")) continue;
                    after = isl_close_bracket(home, after, fn->body_end);
                    if (after == SIZE_MAX) continue;
                    after++;
                }
                if (isl_assignment_op(home, after) || (j >= 3 && (isl_punct(home, j - 3, "++") || isl_punct(home, j - 3, "--")))) {
                    isl_put_str(why, "const-write ");
                    isl_put(why, isl_tok(home, j).ptr, isl_tok(home, j).len);
                    return false;
                }
            }
            continue;
        }
        /* a known keel call: `alias.verb(x, ...)` or `verb(x, ...)` of the module */
        if (!isl_ident(home, j) || !isl_punct(home, j + 1, "(")) continue;
        if (j > fn->body_first && isl_punct(home, j - 1, "->")) continue;
        const KAst *callee = home;
        if (j > fn->body_first + 1 && isl_punct(home, j - 1, ".")) {
            if (!isl_ident(home, j - 2)) continue;
            const KModule *m = isl_module_alias(home->symbols, isl_tok(home, j - 2));
            callee = m ? isl_ast_of(c, m) : NULL;
            if (!callee) continue;
        }
        size_t close = isl_close_paren(home, j + 1, fn->body_end);
        if (close == SIZE_MAX) continue;
        Range first = {0, 0};
        size_t arity = isl_call_args(home, j + 1, close, &first);
        if (!arity) continue;
        while (first.first < first.end && (isl_punct(home, first.first, "&") || isl_punct(home, first.first, "*"))) first.first++;
        if (first.end != first.first + 1 || !isl_ident(home, first.first)) continue;
        keel_slice_char text;
        if (!declared_type(home, fn, isl_tok(home, first.first), j, &text)) continue;
        int saved = c->type_count;
        int obj = isl_kt_from_home(c, home, text, t);
        if (obj >= 0 && isl_kt(c, obj)->kind == KT_MODIFIER && isl_kt(c, obj)->module && isl_ast_of(c, isl_kt(c, obj)->module) == callee) {
            Sig sig = isl_find_verb(callee, isl_tok(home, j), arity);
            const char *inner = NULL;
            if (sig.found && !sig.p[0].is_protocol && !isl_verb_available(c, callee, sig.fn, obj, &inner)) {
                isl_put_str(why, "calls ");
                isl_put_str(why, isl_kt(c, obj)->symbol);
                isl_put_str(why, ".");
                isl_put(why, isl_tok(home, j).ptr, isl_tok(home, j).len);
                isl_put_str(why, "/");
                isl_put_uint(why, arity);
                c->type_count = saved;
                return false;
            }
        }
        c->type_count = saved;
    }
    return true;
}

bool isl_verb_available(Ctx *c, const KAst *home, const KAstNode *fn, int t, const char **cause) {
    const char *inst = isl_kt_symbol(c, t);
    if (!*inst) return true;
    for (size_t i = 0; i < c->surface_count; i++) {
        Surface *s = &c->surface[i];
        if (s->fn != fn || strcmp(s->inst, inst)) continue;
        *cause = s->cause;
        return s->state != 2;               /* a verb still being computed is a cycle: no cause on it */
    }
    if (c->surface_count == K_SURFACE_MAX) return true;
    size_t at = c->surface_count++;
    Surface *s = &c->surface[at];
    memset(s, 0, sizeof *s);
    strncpy(s->inst, inst, sizeof s->inst - 1);
    s->fn = fn;
    Text why = { .ok = true };
    bool ok = compute_surface(c, home, fn, t, &why);
    s = &c->surface[at];
    s->state = ok ? 1 : 2;
    size_t n = why.n < sizeof s->cause - 1 ? why.n : sizeof s->cause - 1;
    memcpy(s->cause, why.s, n);
    s->cause[n] = 0;
    *cause = s->cause;
    return ok;
}

/* the instances inside type `t`: itself and its arguments, at any depth */
size_t isl_instances_in(Ctx *c, int t, int *out, size_t n, size_t cap, int depth) {
    if (t < 0 || depth > 8) return n;
    const KType *k = isl_kt(c, t);
    if (k->kind == KT_MODIFIER && *k->symbol && n < cap) out[n++] = t;
    if (k->kind == KT_MODIFIER)
        for (int i = 0; i < k->argc; i++) n = isl_instances_in(c, k->arg[i], out, n, cap, depth + 1);
    return n;
}

/* a copy of the type tree `t` at the top of the pool */
int isl_kt_deep(Ctx *c, const KType *nodes, int t) {
    int copy = isl_kt_new(c);
    if (copy < 0) return -1;
    c->types[copy] = nodes[t];
    if (nodes[t].kind == KT_MODIFIER)
        for (int i = 0; i < nodes[t].argc; i++) {
            int arg = isl_kt_deep(c, nodes, nodes[t].arg[i]);
            if (arg < 0) return -1;
            c->types[copy].arg[i] = arg;
        }
    if (nodes[t].kind == KT_ARRAY && nodes[t].elem >= 0) c->types[copy].elem = isl_kt_deep(c, nodes, nodes[t].elem);
    return copy;
}



/* ---- provenance and invalidation (spec §4.12) ---------------------------- */

/* the local an argument names: a symbol, with `&` or `*` in front */
int isl_local_of(const Ctx *c, Range r) {
    const KAst *a = c->ast;
    while (r.first < r.end && (isl_punct(a, r.first, "&") || isl_punct(a, r.first, "*"))) r.first++;
    if (r.end != r.first + 1 || !isl_ident(a, r.first)) return -1;
    for (size_t i = c->local_count; i-- > 0;)
        if (k_symtab_same_name(c->locals[i].name, isl_tok(a, r.first))) return (int)i;
    return -1;
}

static bool descends(const Ctx *c, int l, int x, int depth) {
    if (depth > 16 || l < 0 || (size_t)l >= c->local_count) return false;
    for (int k = 0; k < c->locals[l].ndeps; k++) {
        int d = c->locals[l].deps[k];
        if (d == x || descends(c, d, x, depth + 1)) return true;
    }
    return false;
}

/* `x` and, when `self`, `x` itself become invalid with everything that comes from it */
static void invalidate(Ctx *c, int x, bool self, keel_slice_char by) {
    for (size_t l = 0; l < c->local_count; l++)
        if (((int)l == x && self) || descends(c, (int)l, x, 0)) {
            c->locals[l].invalid = true;
            c->locals[l].told = false;
            c->locals[l].invalid_by = by;
        }
}

/* `p` is the product of `parents`: new dependencies, valid again (rules 2, 4, 9) */
void isl_produce(Ctx *c, int p, const int *parents, int np) {
    if (p < 0) return;
    Local *l = &c->locals[p];
    l->ndeps = 0;
    l->local_origin = false;
    l->invalid = l->told = false;
    for (int k = 0; k < np && l->ndeps < 8; k++) {
        if (parents[k] < 0 || parents[k] == p) continue;
        l->deps[l->ndeps++] = parents[k];
        if (c->locals[parents[k]].local_origin) l->local_origin = true;
    }
}

/* the roles of a resolved call, at its arguments (rules 2, 6, 7): `callee` is
   the first token of the call, where an assignment's `=` stands before it */
void isl_roles_at_call(Ctx *c, const KAst *home, const Sig *sig, const Range *args, size_t argc, size_t callee,
                          keel_slice_char verb) {
    KAst *a = c->ast;
    if (c->quiet || !sig->found) return;
    int parents[K_PARAMS_MAX], np = 0;
    for (size_t k = 0; k < argc && k < sig->n; k++)
        if (sig->p[k].role == K_ROLE_PARENT) parents[np++] = isl_local_of(c, args[k]);
    for (size_t k = 0; k < argc && k < sig->n; k++) {
        int x = isl_local_of(c, args[k]);
        if (x < 0) continue;
        if (sig->p[k].role == K_ROLE_INVALIDATES) invalidate(c, x, false, verb);
        if (sig->p[k].role == K_ROLE_CONSUMES) invalidate(c, x, true, verb);
    }
    for (size_t k = 0; k < argc && k < sig->n; k++)
        if (sig->p[k].role == K_ROLE_CHILD) isl_produce(c, isl_local_of(c, args[k]), parents, np);
    bool child_return = false;
    for (size_t j = sig->fn->first; j < sig->fn->name_first; j++)
        if (k_token_role(isl_tok(home, j)) == K_ROLE_CHILD) child_return = true;
    if (child_return && callee >= 2 && isl_punct(a, callee - 1, "=") && isl_ident(a, callee - 2))
        isl_produce(c, isl_local_of(c, (Range){ callee - 2, callee - 1 }), parents, np);
}

/* ---- instance (spec §4.3) ---------------------------------------------- */

/* `instance M.mod args;`: the type is a modifier of a generic module
   (instance-not-modifier), and that module has a body to place: a generic
   that is `inline` all through needs none (redundant-instance). */
void isl_instance_decl(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    keel_slice_char text = isl_range_text(a, (Range){ n->name_first, n->name_end });
    KLexer lexer;
    TKPpKind pp;
    KSpecifier spec;
    KToken next = {0};
    KToken first = isl_enter(a, n->name_first, &lexer, &pp);
    if (!k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) || spec.kind != K_SPEC_MODIFIER ||
        (next.len && next.ptr < text.ptr + text.len)) {
        isl_diag3(c, K_DIAG_INSTANCE_NOT_MODIFIER, isl_tok(a, n->name_first), text, none, none);
        return;
    }
    isl_check_arguments(c, &spec, isl_tok(a, n->name_first));
    const KSymbol *m = k_symbol_resolve(a->symbols, spec.modifier_name);
    const KAst *home = m && m->origin ? isl_ast_of(c, m->origin) : NULL;
    if (!home) return;
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *fn = isl_node_at(home, i);
        if ((fn->kind == K_AST_FUNCTION && !fn->is_inline) || fn->kind == K_AST_VARIABLE) return;
    }
    isl_diag3(c, K_DIAG_REDUNDANT_INSTANCE, isl_tok(a, n->first), text, none, none);
}

/* Spec §4.3, rule 20: in a generic module, a declaration that mentions no
   parameter and no modifier belongs to the module and is emitted once, so it
   is a type, a `constexpr` or an `inline` function — never one out of line
   nor a variable (nonparametric-out-of-line). */
static bool own_modifier(const KAst *a, size_t j) {
    if (!isl_ident(a, j)) return false;
    for (size_t k = 0; k < a->nodes.len; k++) {
        const KAstNode *d = isl_node_at(a, k);
        if (d->kind == K_AST_MODIFIER && k_symtab_same_name(isl_tok(a, j), isl_tok(a, d->name_first))) return true;
    }
    return false;
}

static bool type_binder(const KAst *a, KToken t) {
    const KAstNode *m = isl_module_node(a);
    for (size_t k = m->type_first; k < m->type_end; k++) if (isl_ident(a, k) && k_symtab_same_name(isl_tok(a, k), t)) return true;
    return false;
}

/* Spec §4.3, rule 7: inside the generic, a value of a parameter type, or a
   pointer to one, takes no keel construction — it is opaque until the
   substitution (protocol-on-parameter). */
static void check_opaque(Ctx *c, const KAstNode *fn) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (fn->body_first == SIZE_MAX) return;
    keel_slice_char names[32];
    size_t nn = 0;
    Param p[K_PARAMS_MAX];
    size_t np = isl_params_of(a, fn, p, K_PARAMS_MAX);
    for (size_t k = 0; np != SIZE_MAX && k < np && nn < 32; k++) {
        Range r = isl_param_type(a, &p[k]);
        if (!p[k].is_type && r.end == r.first + 1 && type_binder(a, isl_tok(a, r.first)) && isl_ident(a, p[k].end - 1))
            names[nn++] = isl_tok(a, p[k].end - 1);
    }
    for (size_t j = fn->body_first; j + 1 < fn->body_end && nn < 32; j++) {
        if (!type_binder(a, isl_tok(a, j)) || !(isl_punct(a, j - 1, ";") || isl_punct(a, j - 1, "{") || isl_punct(a, j - 1, "}") ||
                                            isl_punct(a, j - 1, "(")))
            continue;
        size_t d = j + 1;
        while (isl_punct(a, d, "*")) d++;
        if (isl_ident(a, d)) names[nn++] = isl_tok(a, d);
    }
    if (!nn) return;
    for (size_t j = fn->body_first; j < fn->body_end; j++) {
        const char *what = NULL;
        size_t at = j, x = SIZE_MAX;
        if ((isl_word(a, j, "foreach") || isl_word(a, j, "walk")) && isl_punct(a, j + 1, "(")) {
            size_t close = isl_close_paren(a, j + 1, fn->body_end);
            if (close != SIZE_MAX && isl_punct(a, close - 2, ":")) { x = close - 1; what = isl_word(a, j, "foreach") ? "foreach" : "walk"; }
        } else if (isl_word(a, j, "match") && isl_punct(a, j + 1, "(") && isl_punct(a, j + 3, ")")) {
            x = j + 2; what = "match";
        } else if (isl_ident(a, j) && isl_punct(a, j + 1, "[")) {
            size_t close = isl_close_bracket(a, j + 1, fn->body_end);
            for (size_t k = j + 2; close != SIZE_MAX && k < close; k++) if (isl_punct(a, k, "..")) { x = j; what = "range-index"; }
        } else if (isl_ident(a, j) && isl_punct(a, j + 1, ".") && isl_ident(a, j + 2) && isl_punct(a, j + 3, "(") &&
                   isl_module_alias(a->symbols, isl_tok(a, j))) {
            size_t arg = j + 4;
            while (isl_punct(a, arg, "&") || isl_punct(a, arg, "*")) arg++;
            if (isl_punct(a, arg + 1, ",") || isl_punct(a, arg + 1, ")")) { x = arg; what = "qualified call"; }
        } else if (isl_word(a, j, "else") && !isl_punct(a, j - 1, ";") && !isl_punct(a, j - 1, "}")) {
            for (size_t k = j; k > fn->body_first && !isl_punct(a, k - 1, ";") && !isl_punct(a, k - 1, "{"); k--)
                if (isl_punct(a, k, "=") && isl_ident(a, k - 1)) { x = k - 1; what = "else"; break; }
        }
        if (x == SIZE_MAX || !isl_ident(a, x)) continue;
        for (size_t k = 0; k < nn; k++)
            if (k_symtab_same_name(names[k], isl_tok(a, x))) {
                isl_diag3(c, K_DIAG_PROTOCOL_ON_PARAMETER, isl_tok(a, at), k_diag_text(what), isl_tok(a, x), none);
                break;
            }
    }
}

void isl_check_generic(Ctx *c) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    const KAstNode *m = isl_module_node(a);
    /* the module instantiating itself without end: its own modifier applied
       over its own modifier, `stack stack T`, in a signature or a field
       (circular-generic) */
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind != K_AST_FUNCTION && n->kind != K_AST_MODIFIER) continue;
        size_t end = n->kind == K_AST_FUNCTION && n->body_first != SIZE_MAX ? n->body_first : n->end;
        size_t from = n->kind == K_AST_MODIFIER ? n->name_end : n->first;
        for (size_t j = from; j + 1 < end; j++)
            if (own_modifier(a, j) && own_modifier(a, j + 1)) {
                Text chain = { .ok = true };
                isl_put_range(&chain, a, j, j + 2);
                isl_put_str(&chain, " needs ");
                isl_put(&chain, isl_tok(a, j).ptr, isl_tok(a, j).len);
                isl_put_str(&chain, " ");
                isl_put_range(&chain, a, j, j + 2);
                isl_put_str(&chain, ", and so on");
                isl_diag3(c, K_DIAG_CIRCULAR_GENERIC, isl_tok(a, j), isl_keep(c, &chain), none, none);
                break;
            }
        if (n->kind == K_AST_FUNCTION) check_opaque(c, n);
    }
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (!((n->kind == K_AST_FUNCTION && !n->is_inline) || n->kind == K_AST_VARIABLE)) continue;
        bool mentions = false;
        size_t end = n->body_first != SIZE_MAX ? n->body_first : n->end;
        for (size_t j = n->first; j < end && !mentions; j++) {
            if (!isl_ident(a, j)) continue;
            for (size_t k = m->dim_first; k < m->type_end; k++)
                if (isl_ident(a, k) && k_symtab_same_name(isl_tok(a, j), isl_tok(a, k))) mentions = true;
            for (size_t k = 0; k < a->nodes.len && !mentions; k++) {
                const KAstNode *d = isl_node_at(a, k);
                if (d->kind == K_AST_MODIFIER && k_symtab_same_name(isl_tok(a, j), isl_tok(a, d->name_first))) mentions = true;
            }
        }
        if (!mentions)
            isl_diag3(c, K_DIAG_NONPARAMETRIC_OUT_OF_LINE, isl_tok(a, n->name_first), isl_tok(a, n->name_first), none, none);
    }
}

/* a modifier is named `instance` (modifier-named-instance), or stands in a
   module with no parameters (modifier-outside-generic) */
void isl_check_modifiers(Ctx *c) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind != K_AST_MODIFIER) continue;
        KToken name = isl_tok(a, n->name_first);
        if (k_token_spelled(name, "instance")) isl_diag3(c, K_DIAG_MODIFIER_NAMED_INSTANCE, name, name, none, none);
        if (!isl_generic(a)) isl_diag3(c, K_DIAG_MODIFIER_OUTSIDE_GENERIC, name, name, none, none);
    }
}
