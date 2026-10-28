/* Pass 3, the container types, the names and the declarations of the file: the pool of `KType`, and the declarations read before the islands are printed. Split from parser_islands.c by T3; the code is unchanged. */

#include "engine/parser/parser_islands_internal.h"

/* ---- container types ---------------------------------------------------- */

KType *isl_kt(Ctx *c, int t) { return &c->types[t]; }

/* the symbol of a type in the pool; empty for one that could not be made */
const char *isl_kt_symbol(const Ctx *c, int t) { return t >= 0 ? c->types[t].symbol : ""; }

int isl_kt_new(Ctx *c) {
    if (c->type_count == K_TYPES_MAX) return -1;
    c->types[c->type_count] = (KType){ .elem = -1 };
    return c->type_count++;
}

static int kt_copy(Ctx *c, int t) {
    if (t < 0) return -1;
    int n = isl_kt_new(c);
    if (n >= 0) c->types[n] = c->types[t];
    return n;
}

void isl_kt_set_symbol(KType *k, const char *s, size_t n) {
    if (n < K_SYMBOL_MAX) { memcpy(k->symbol, s, n); k->symbol[n] = 0; }
}

/* the module's own AST; the file being read has no KModule yet */
const KAst *isl_ast_of(const Ctx *c, const KModule *m) { return m && m->ast ? m->ast : c->ast; }

/* the module whose retained tree is `home`, through the file's table */
static const KModule *ast_module(const Ctx *c, const KAst *home) {
    for (size_t i = 0; i < c->ast->symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(c->ast->symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin && s->origin->ast == home) return s->origin;
    }
    return NULL;
}

/* the C symbol of a modifier's instance: the modifier's name and its arguments' */
static void kt_compose(Ctx *c, int t, const KSymbol *modifier) {
    KType *k = isl_kt(c, t);
    char buf[K_SYMBOL_MAX];
    size_t n = k_mangle_symbol(modifier, (keel_slice_char){ sizeof buf, buf });
    if (n > sizeof buf) return;
    for (int i = 0; i < k->argc; i++) {
        const char *s = isl_kt_symbol(c, k->arg[i]);
        size_t len = strlen(s);
        if (!len || n + 1 + len >= sizeof buf) return;
        buf[n++] = '_';
        memcpy(buf + n, s, len);
        n += len;
    }
    isl_kt_set_symbol(k, buf, n);
}

/* A type written in `symbols`' scope. */
int isl_kt_from_text(Ctx *c, const KSymbolTable *symbols, keel_slice_char text) {
    int t = isl_kt_new(c);
    if (t < 0) return -1;
    char sym[K_SYMBOL_MAX];
    size_t n = k_spec_symbol(text, symbols, sym, sizeof sym);
    if (n != SIZE_MAX) isl_kt_set_symbol(isl_kt(c, t), sym, n);
    KLexer lexer;
    TKPpKind pp;
    KToken first = isl_enter_text(text, &lexer, &pp);
    while (first.len && (k_token_spelled(first, "const") || k_token_spelled(first, "volatile") ||
                         k_token_spelled(first, "_Atomic")))
        first = k_lexer_next(&lexer, &pp);
    KSpecifier spec;
    KToken next = {0};
    if (!first.len || !k_token_is_ident(first) ||
        !k_scan_known_type(&lexer, first, symbols, &spec, &next, &pp) || spec.kind == K_SPEC_NONE) {
        isl_kt(c, t)->kind = n != SIZE_MAX ? KT_PRIM : KT_NONE;       /* `char`, `struct S`, ... */
        return t;
    }
    if (spec.kind == K_SPEC_NAMED_TYPE) {
        const KSymbol *s = k_symbol_resolve(symbols, spec.type_name);
        isl_kt(c, t)->kind = k_primitive_name(spec.type_name) ? KT_PRIM : KT_NAMED;
        if (s) { isl_kt(c, t)->module = s->origin; isl_kt(c, t)->name = s->name; }
        return t;
    }
    const KSymbol *m = k_symbol_resolve(symbols, spec.modifier_name);
    isl_kt(c, t)->kind = KT_MODIFIER;
    if (m) { isl_kt(c, t)->module = m->origin; isl_kt(c, t)->name = m->name; }
    for (size_t i = 0; i < spec.arg_count; i++) {
        int arg = isl_kt_from_text(c, symbols, spec.args[i]);
        if (isl_kt(c, t)->argc < K_SPEC_MAX_ARGS) isl_kt(c, t)->arg[isl_kt(c, t)->argc++] = arg;
    }
    return t;
}

static int kt_of_local(Ctx *c, const Local *l) {
    int t;
    if (l->rank > 0) {
        t = isl_kt_new(c);
        if (t < 0) return -1;
        int element = isl_kt_from_text(c, c->ast->symbols, l->element);
        isl_kt(c, t)->kind = KT_ARRAY;
        isl_kt(c, t)->rank = l->rank;
        isl_kt(c, t)->dims = l->dims;
        isl_kt(c, t)->elem = element;
    } else if (l->spec.kind == K_SPEC_NONE) {
        t = isl_kt_new(c);
    } else {
        t = isl_kt_from_text(c, c->ast->symbols, l->spec.text);
    }
    if (t >= 0) {
        isl_kt(c, t)->pointers = l->rank > 0 ? 0 : l->pointers;
        isl_kt(c, t)->lvalue = true;
    }
    return t;
}

/* the type parameters of a generic module, in the order of its `module` line */
size_t isl_binders_of(const KAst *home, keel_slice_char out[8]) {
    const KAstNode *m = isl_module_node(home);
    size_t n = 0;
    for (size_t j = m->tags_first; j < m->tags_end; j++) if (isl_ident(home, j) && n < 8) out[n++] = isl_tok(home, j);
    for (size_t j = m->type_first; j < m->type_end; j++) if (isl_ident(home, j) && n < 8) out[n++] = isl_tok(home, j);
    return n;
}

/* A type written in a generic module `home`, seen from an instance of it: the
   type parameters are the instance's arguments, and the module's own modifier,
   or an imported one written without arguments, takes them as they stand. */
int isl_kt_from_home(Ctx *c, const KAst *home, keel_slice_char text, int inst) {
    KLexer lexer;
    TKPpKind pp;
    KToken first = isl_enter_text(text, &lexer, &pp);
    while (first.len && (k_token_spelled(first, "const") || k_token_spelled(first, "volatile") ||
                         k_token_spelled(first, "_Atomic")))
        first = k_lexer_next(&lexer, &pp);
    if (!first.len) return -1;
    keel_slice_char body = { (size_t)(text.ptr + text.len - first.ptr), first.ptr };
    KToken after = k_lexer_next(&lexer, &pp);
    keel_slice_char binders[8];
    size_t nb = isl_binders_of(home, binders);

    /* `slice.slice`, an imported modifier written qualified and without
       arguments, takes them as they stand, like the bare `slice` (spec §4.3) */
    if (k_token_is_punct(after, ".")) {
        KLexer look;
        KToken rest;
        TKPpKind lpp;
        KToken head = isl_enter_text(body, &look, &lpp);
        keel_slice_char qualified = k_scan_qualified_name(&look, head, &rest, &lpp);
        const KSymbol *s = rest.len ? NULL : k_symbol_resolve(home->symbols, qualified);
        if (s && s->kind == K_SYM_MODIFIER && s->origin) {
            int t = isl_kt_new(c);
            if (t < 0) return -1;
            isl_kt(c, t)->kind = KT_MODIFIER;
            isl_kt(c, t)->module = s->origin;
            isl_kt(c, t)->name = s->name;
            for (int i = 0; i < s->arity - s->dim_arity && i < K_SPEC_MAX_ARGS; i++) {
                int arg = inst >= 0 && i < isl_kt(c, inst)->argc ? kt_copy(c, isl_kt(c, inst)->arg[i]) : -1;
                if (arg < 0) return -1;
                isl_kt(c, t)->arg[isl_kt(c, t)->argc++] = arg;
            }
            kt_compose(c, t, s);
            return t;
        }
    }

    if (!after.len) {                                   /* one name */
        for (size_t p = 0; p < nb; p++)
            if (k_symtab_same_name(binders[p], first)) {
                if (inst < 0 || p >= (size_t)isl_kt(c, inst)->argc) return -1;
                return kt_copy(c, isl_kt(c, inst)->arg[p]);
            }
        if (k_symtab_same_name(first, isl_modifier_of(home))) return kt_copy(c, inst);
        /* the module's own type wins over one `types` injected (spec §4.1):
           `cursor` in keel.buffer is keel.buffer's, not keel.slice's */
        for (size_t i = 0; i < home->nodes.len; i++) {
            const KAstNode *d = isl_node_at(home, i);
            if (d->kind != K_AST_TYPE || !k_symtab_same_name(isl_tok(home, d->name_first), first)) continue;
            int t = isl_kt_new(c);
            if (t < 0) return -1;
            isl_kt(c, t)->kind = KT_NAMED;
            isl_kt(c, t)->name = first;
            for (size_t k = 0; home->symbols && k < home->symbols->len; k++) {
                const KSymbol *q = keel_buffer_KSymbol_ptr(home->symbols, k);
                if (q->kind == K_SYM_TYPE && k_symtab_same_name(q->name, first) && q->origin &&
                    q->origin->ast == home) {
                    char buf[K_SYMBOL_MAX];
                    size_t n = k_mangle_symbol(q, (keel_slice_char){ sizeof buf, buf });
                    if (n < sizeof buf) isl_kt_set_symbol(isl_kt(c, t), buf, n);
                    isl_kt(c, t)->module = q->origin;
                }
            }
            return t;
        }
        const KSymbol *s = k_symbol_resolve(home->symbols, first);
        if (s && s->kind == K_SYM_MODIFIER && s->origin) {
            int t = isl_kt_new(c);
            if (t < 0) return -1;
            isl_kt(c, t)->kind = KT_MODIFIER;
            isl_kt(c, t)->module = s->origin;
            isl_kt(c, t)->name = s->name;
            for (int i = 0; i < s->arity - s->dim_arity && i < K_SPEC_MAX_ARGS; i++) {
                int arg = inst >= 0 && i < isl_kt(c, inst)->argc ? kt_copy(c, isl_kt(c, inst)->arg[i]) : -1;
                if (arg < 0) return -1;
                isl_kt(c, t)->arg[isl_kt(c, t)->argc++] = arg;
            }
            kt_compose(c, t, s);
            return t;
        }
        return isl_kt_from_text(c, home->symbols, body);
    }

    KLexer again;
    KSpecifier spec;
    KToken next = {0};
    KToken head = isl_enter_text(body, &again, &pp);
    if (!k_token_is_ident(head) || !k_scan_known_type(&again, head, home->symbols, &spec, &next, &pp) ||
        spec.kind != K_SPEC_MODIFIER) {
        /* `outcome buffer` in keel.buffer: a one-argument modifier over the
           module's own modifier, written bare, which the scanner does not take */
        const KSymbol *m = k_token_is_ident(head) ? k_symbol_resolve(home->symbols, head) : NULL;
        if (m && m->kind == K_SYM_MODIFIER && m->origin && m->arity - m->dim_arity == 1) {
            keel_slice_char rest = { (size_t)(body.ptr + body.len - after.ptr), after.ptr };
            int arg = isl_kt_from_home(c, home, rest, inst);
            int t = arg >= 0 ? isl_kt_new(c) : -1;
            if (t >= 0 && *isl_kt(c, arg)->symbol) {
                isl_kt(c, t)->kind = KT_MODIFIER;
                isl_kt(c, t)->module = m->origin;
                isl_kt(c, t)->name = m->name;
                isl_kt(c, t)->arg[isl_kt(c, t)->argc++] = arg;
                kt_compose(c, t, m);
                return t;
            }
        }
        return isl_kt_from_text(c, home->symbols, body);
    }
    const KSymbol *m = k_symbol_resolve(home->symbols, spec.modifier_name);
    if (!m || !m->origin) return -1;
    int t = isl_kt_new(c);
    if (t < 0) return -1;
    isl_kt(c, t)->kind = KT_MODIFIER;
    isl_kt(c, t)->module = m->origin;
    isl_kt(c, t)->name = m->name;
    for (size_t i = 0; i < spec.arg_count; i++) {
        int arg = isl_kt_from_home(c, home, spec.args[i], inst);
        if (arg < 0 || isl_kt(c, t)->argc == K_SPEC_MAX_ARGS) return -1;
        isl_kt(c, t)->arg[isl_kt(c, t)->argc++] = arg;
    }
    kt_compose(c, t, m);
    return t;
}

/* the declared return type of `fn`, for an instance `inst` of the module that declares it */
int isl_ret_type(Ctx *c, const KAst *home, const KAstNode *fn, int inst) {
    size_t s = fn->first;
    while (s < fn->name_first && (isl_word(home, s, "pub") || isl_word(home, s, "priv") || isl_word(home, s, "inline") ||
                                  isl_word(home, s, "static") || isl_word(home, s, "extern") ||
                                  k_token_role(isl_tok(home, s)) == K_ROLE_CHILD))
        s++;
    size_t e = fn->name_first;
    int pointers = 0;
    while (e > s && isl_punct(home, e - 1, "*")) { pointers++; e--; }
    if (e <= s) return -1;
    int t = isl_kt_from_home(c, home, isl_range_text(home, (Range){ s, e }), inst);
    if (t >= 0) {
        isl_kt(c, t)->pointers = pointers;
        isl_kt(c, t)->lvalue = false;
    }
    return t;
}

/* the instance of module `m` inside a type, found where a parameter that is not
   the module's own modifier still mentions it (`slice slot s`) */
static int kt_find(Ctx *c, int t, const KModule *m, int depth) {
    if (t < 0 || depth > 8) return -1;
    KType *k = isl_kt(c, t);
    if (k->kind == KT_MODIFIER && k->module == m) return t;
    if (k->kind == KT_MODIFIER)
        for (int i = 0; i < k->argc; i++) {
            int found = kt_find(c, k->arg[i], m, depth + 1);
            if (found >= 0) return found;
        }
    if (k->kind == KT_ARRAY) return kt_find(c, k->elem, m, depth + 1);
    return -1;
}

/* A field of a struct the file or an import declares: the keel ones are
   registered with their types, the C ones are opaque (spec §4.2). */
static int scan_fields(Ctx *c, const KAst *ast, size_t first, size_t last, keel_slice_char field) {
    for (size_t j = first; j < last; ) {
        size_t e = j, depth = 0;
        while (e < last && !(depth == 0 && isl_punct(ast, e, ";"))) {
            if (isl_punct(ast, e, "(") || isl_punct(ast, e, "[") || isl_punct(ast, e, "{")) depth++;
            else if (isl_punct(ast, e, ")") || isl_punct(ast, e, "]") || isl_punct(ast, e, "}")) depth--;
            e++;
        }
        size_t k = j;
        if (isl_word(ast, k, "alignas") && isl_punct(ast, k + 1, "(")) {
            size_t cp = isl_close_paren(ast, k + 1, e);
            if (cp != SIZE_MAX) k = cp + 1;
        }
        KLexer lexer;
        TKPpKind pp;
        if (k < e && isl_word(ast, k, "array")) {
            KToken next = {0}, head = isl_enter(ast, k + 1, &lexer, &pp);
            keel_slice_char element;
            if (k_scan_argument(&lexer, head, ast->symbols, &element, &next, &pp))
                for (KToken d = next; d.len; ) {
                    KDeclarator decl;
                    KToken after;
                    if (!(k_token_is_punct(d, "*") || k_token_is_ident(d))) break;
                    if (!k_scan_declarator(&lexer, d, &decl, &after, &pp) || !decl.name.len || !decl.has_array_suffix) break;
                    if (k_symtab_same_name(decl.name, field)) {
                        const char *dims = decl.name.ptr + decl.name.len, *end = after.len ? after.ptr : dims;
                        int t = isl_kt_new(c);
                        if (t < 0) return -1;
                        isl_kt(c, t)->kind = KT_ARRAY;
                        isl_kt(c, t)->rank = isl_rank_of(dims, end);
                        isl_kt(c, t)->dims = (keel_slice_char){ (size_t)(end - dims), (char *)dims };
                        isl_kt(c, t)->elem = isl_kt_from_text(c, ast->symbols, element);
                        isl_kt(c, t)->lvalue = true;
                        return t;
                    }
                    if (!k_token_is_punct(after, ",")) break;
                    d = k_lexer_next(&lexer, &pp);
                }
        } else if (k < e && isl_ident(ast, k)) {
            KSpecifier spec;
            KToken next = {0}, head = isl_enter(ast, k, &lexer, &pp);
            if (k_scan_known_type(&lexer, head, ast->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE)
                for (KToken d = next; d.len; ) {
                    KDeclarator decl;
                    KToken after;
                    if (!(k_token_is_punct(d, "*") || k_token_is_ident(d))) break;
                    if (!k_scan_declarator(&lexer, d, &decl, &after, &pp) || !decl.name.len) break;
                    if (k_symtab_same_name(decl.name, field)) {
                        int t = isl_kt_from_text(c, ast->symbols, spec.text);
                        if (t >= 0) { isl_kt(c, t)->pointers = decl.pointer_depth; isl_kt(c, t)->lvalue = true; }
                        return t;
                    }
                    if (!k_token_is_punct(after, ",")) break;
                    d = k_lexer_next(&lexer, &pp);
                }
        }
        j = e + 1;
    }
    return -1;
}

static int field_type(Ctx *c, int t, keel_slice_char field) {
    if (t < 0) return -1;
    KType *k = isl_kt(c, t);
    if (k->kind != KT_NAMED || !k->name.len) return -1;
    const KAst *ast = isl_ast_of(c, k->module);
    if (!ast || !ast->symbols) return -1;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = isl_node_at(ast, i);
        if (n->kind != K_AST_TYPE || !k_symtab_same_name(isl_tok(ast, n->name_first), k->name)) continue;
        size_t open = n->first;
        while (open < n->end && !isl_punct(ast, open, "{")) open++;
        if (open == n->end) return -1;
        size_t depth = 0, close = open;
        for (; close < n->end; close++) {
            if (isl_punct(ast, close, "{")) depth++;
            else if (isl_punct(ast, close, "}") && --depth == 0) break;
        }
        return scan_fields(c, ast, open + 1, close, field);
    }
    return -1;
}

/* `x[i]` with `count` indices: the element of an array, or what `ptr` points to */
static int index_type(Ctx *c, int t, size_t count) {
    if (t < 0) return -1;
    KType *k = isl_kt(c, t);
    if (k->kind == KT_ARRAY) {
        if (count > (size_t)k->rank) return -1;
        if (count == (size_t)k->rank) {
            int e = kt_copy(c, k->elem);
            if (e >= 0) isl_kt(c, e)->lvalue = true;
            return e;
        }
        int rest = kt_copy(c, t);
        if (rest >= 0) isl_kt(c, rest)->rank -= (int)count;
        return rest;
    }
    if (k->kind == KT_MODIFIER && k->module) {
        const KAst *home = isl_ast_of(c, k->module);
        Sig sig = isl_find_verb(home, k_diag_text("ptr"), count + 1);
        if (!sig.found) return -1;
        int p = isl_ret_type(c, home, sig.fn, t);
        if (p < 0 || isl_kt(c, p)->pointers < 1) return -1;
        isl_kt(c, p)->pointers--;
        isl_kt(c, p)->lvalue = true;
        return p;
    }
    return -1;
}

static bool first_arg(const KAst *a, size_t open, Range *out) {
    size_t depth = 0, j = open + 1;
    for (; j < a->tokens.len; j++) {
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}")) { if (depth == 0) break; depth--; }
        else if (depth == 0 && isl_punct(a, j, ",")) break;
    }
    if (j == open + 1) return false;
    *out = (Range){ open + 1, j };
    return true;
}

int isl_type_of(Ctx *c, Range r);
static const KModule *ast_module(const Ctx *c, const KAst *home);
bool isl_tag_of_some_set(const KAst *where, keel_slice_char name);
static int call_core(Ctx *c, size_t callee, size_t verb, size_t open, const KModule *module);

/* How a call written at token `i` is dispatched: `alias.verb(`; `verb(x, …)`
   as the verb of the type of the container `x` (spec §4.4, item 1); or a
   function of the file itself. */
typedef struct { size_t verb, open; const KModule *module; } Callee;

static bool callee_at(Ctx *c, size_t i, Callee *out) {
    const KAst *a = c->ast;
    KToken name = isl_tok(a, i);
    if (!isl_ident(a, i)) return false;
    /* `alias.verb(`: a local of the same name does not hide the qualifier,
       which is read where it stands as one (spec §2.3) */
    if (isl_punct(a, i + 1, ".") && isl_ident(a, i + 2) && isl_punct(a, i + 3, "(")) {
        const KModule *m = isl_module_alias(a->symbols, name);
        if (!m) return false;
        *out = (Callee){ i + 2, i + 3, m };
        return true;
    }
    if (isl_find_local(c, name)) return false;
    if (!isl_punct(a, i + 1, "(")) return false;
    Range first;
    if (first_arg(a, i + 1, &first)) {
        int t = isl_type_of(c, first);
        if (t >= 0 && isl_kt(c, t)->kind == KT_MODIFIER && isl_kt(c, t)->module) {
            const KAst *home = isl_ast_of(c, isl_kt(c, t)->module);
            if (isl_find_verb(home, name, SIZE_MAX).found) {
                *out = (Callee){ i, i + 1, isl_kt(c, t)->module };
                return true;
            }
        }
    }
    for (size_t k = 0; k < a->nodes.len; k++)
        if (isl_node_at(a, k)->kind == K_AST_FUNCTION && k_symtab_same_name(isl_tok(a, isl_node_at(a, k)->name_first), name)) {
            *out = (Callee){ i, i + 1, NULL };
            return true;
        }
    return false;
}

/* the type of a call's result, without printing or refusing anything */
static int call_result(Ctx *c, size_t callee, size_t verb, size_t open, const KModule *module) {
    bool quiet = c->quiet;
    c->quiet = true;
    int t = call_core(c, callee, verb, open, module);
    c->quiet = quiet;
    return t;
}

/* The type of an expression in container position (spec §2.2, `container`):
   a name, `*x`, `&x`, `(x)`, `x[i]`, `x.f`, `x->f`, and a verb over a container.
   -1 when it is none of these, or names something this pass has not seen declared. */
int isl_type_of(Ctx *c, Range r) {
    const KAst *a = c->ast;
    while (r.end > r.first + 1 && isl_punct(a, r.first, "(") && isl_close_paren(a, r.first, r.end) == r.end - 1) {
        r.first++;
        r.end--;
    }
    if (r.first >= r.end) return -1;
    if (isl_punct(a, r.first, "*") || isl_punct(a, r.first, "&")) {
        bool star = isl_punct(a, r.first, "*");
        int t = isl_type_of(c, (Range){ r.first + 1, r.end });
        if (t < 0) return -1;
        if (star) {
            if (isl_kt(c, t)->pointers < 1) return -1;
            isl_kt(c, t)->pointers--;
            isl_kt(c, t)->lvalue = true;
        } else {
            isl_kt(c, t)->pointers++;
            isl_kt(c, t)->lvalue = false;
        }
        return t;
    }
    if (!isl_ident(a, r.first)) return -1;
    size_t j = r.first;
    int t = -1;
    Callee ce;
    if (callee_at(c, j, &ce)) {
        size_t close = isl_close_paren(a, ce.open, r.end);
        if (close == SIZE_MAX) return -1;
        t = call_result(c, j, ce.verb, ce.open, ce.module);
        j = close + 1;
    } else {
        const Local *l = isl_find_local(c, isl_tok(a, j));
        if (!l) return -1;
        t = kt_of_local(c, l);
        j++;
    }
    while (t >= 0 && j < r.end) {
        if (isl_punct(a, j, "[")) {
            size_t close = isl_close_bracket(a, j, r.end);
            if (close == SIZE_MAX) return -1;
            size_t count = 1;
            for (size_t k = j + 1, d = 0; k < close; k++) {
                if (isl_punct(a, k, "(") || isl_punct(a, k, "[") || isl_punct(a, k, "{")) d++;
                else if (isl_punct(a, k, ")") || isl_punct(a, k, "]") || isl_punct(a, k, "}")) d--;
                else if (d == 0 && isl_punct(a, k, ",")) count++;
                else if (d == 0 && isl_punct(a, k, "..")) return -1;            /* the view: not typed */
            }
            t = index_type(c, t, count);
            j = close + 1;
        } else if ((isl_punct(a, j, ".") || isl_punct(a, j, "->")) && isl_ident(a, j + 1)) {
            t = field_type(c, t, isl_tok(a, j + 1));
            j += 2;
        } else {
            return -1;
        }
    }
    return j == r.end ? t : -1;
}

void isl_resolution_begin(Ctx *c) { if (c->nest++ == 0) c->type_count = 0; }
void isl_resolution_end(Ctx *c) { c->nest--; }

/* The instance a call's verb belongs to is named by the module and the
   arguments: `keel_routine_ag2_Ag` for `seq`, where the receiver is a table of
   slots and the modifier is not the first parameter. A verb whose object is the
   modifier itself takes the type's own symbol instead: `keel_routine_slot_ag2_Ag_code`. */
static bool module_instance_prefix(Ctx *c, const KAst *home, int inst, Text *t) {
    isl_put_module_prefix(t, home);
    for (int i = 0; inst >= 0 && i < isl_kt(c, inst)->argc; i++) {
        const char *s = isl_kt_symbol(c, isl_kt(c, inst)->arg[i]);
        if (!*s) return false;
        isl_put_str(t, "_");
        isl_put_str(t, s);
    }
    return t->ok;
}

static bool mentions_own(const KAst *home, const Param *p) {
    keel_slice_char own = isl_modifier_of(home);
    for (size_t k = p->first; own.len && k < p->end; k++)
        if (isl_ident(home, k) && k_symtab_same_name(isl_tok(home, k), own)) return true;
    return false;
}

static int prim_type(Ctx *c, const char *name) {
    int t = isl_kt_new(c);
    if (t < 0) return -1;
    isl_kt(c, t)->kind = KT_PRIM;
    isl_kt_set_symbol(isl_kt(c, t), name, strlen(name));
    return t;
}

/* One call: `callee` is the first token of the written callee, `verb` its
   last, `open` the '(' — `module` is the qualifier's module, NULL for a call
   to the file's own function. The island is always printed, resolved or not:
   what the module does not declare goes on as the qualified call, for the C
   compiler to validate (spec §4.4, item 3). It is not printed only where this
   pass cannot name the instance: the object is a shape it does not type yet.
   Returns the type of the result, or -1. */
/* Spec §4.2, rules 12 and 22: the argument of an `array` parameter with a
   binder is an `array` symbol or field (binder-argument-not-array), and an
   `array` argument has the parameter's rank, the same dimensions from index 1
   on, and a dimension 0 no smaller than the declared one
   (array-argument-wrong-dimension). */
static void check_array_argument(Ctx *c, const KAst *home, const Param *p, Range arg, const KType *ak) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t open = SIZE_MAX;
    for (size_t j = p->first; j < p->end; j++) if (isl_punct(home, j, "[")) { open = j; break; }
    if (open == SIZE_MAX) return;
    bool binder = isl_word(home, open + 1, "size_t") && isl_ident(home, open + 2);
    if (!ak || ak->kind != KT_ARRAY) {
        if (binder && arg.end > arg.first)
            isl_diag3(c, K_DIAG_BINDER_ARGUMENT_NOT_ARRAY, isl_tok(a, arg.first), isl_range_text(a, arg), none, none);
        return;
    }
    keel_slice_char dims = isl_range_text(home, (Range){ open, p->end });
    int rank = isl_rank_of(dims.ptr, dims.ptr + dims.len);
    long want[8], got[8];
    size_t nw = isl_dims_of(c, dims, want), ng = isl_dims_of(c, ak->dims, got);
    bool wrong = rank != ak->rank;
    for (size_t d = 1; !wrong && d < nw && d < ng && d < 8; d++)
        if (want[d] >= 0 && got[d] >= 0 && want[d] != got[d]) wrong = true;
    if (!wrong && !binder && nw && ng && want[0] >= 0 && got[0] >= 0 && got[0] < want[0]) wrong = true;
    if (wrong) isl_diag3(c, K_DIAG_ARRAY_ARGUMENT_WRONG_DIMENSION, isl_tok(a, arg.first), isl_range_text(a, arg), ak->dims, dims);
}

static int call_core(Ctx *c, size_t callee, size_t verb, size_t open, const KModule *module) {
    KAst *a = c->ast;
    size_t close = isl_close_paren(a, open, a->tokens.len);
    if (close == SIZE_MAX) return -1;

    Range args[K_PARAMS_MAX];
    size_t argc = 0, start = open + 1, depth = 0;
    for (size_t j = open + 1; j <= close; j++) {
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{")) depth++;
        else if (j < close && (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}"))) depth--;
        else if (j == close || (depth == 0 && isl_punct(a, j, ","))) {
            if (j > start || argc > 0) {
                if (argc == K_PARAMS_MAX) return -1;
                args[argc++] = (Range){ start, j };
            }
            start = j + 1;
        }
    }

    const KAst *home = module ? isl_ast_of(c, module) : a;
    if (!home) return -1;
    KToken name = isl_tok(a, verb);
    keel_slice_char written = isl_range_text(a, (Range){ callee, verb + 1 });
    int t0 = argc ? isl_type_of(c, args[0]) : -1;
    const KType *k0 = t0 >= 0 ? isl_kt(c, t0) : NULL;
    keel_slice_char none = {0};

    /* the core's `length`, `capacity` and `dim` lower to no function (backend §5.2):
       the island says `core`, and the dimension of the array comes from the table */
    if (module && isl_module_named(home, "keel")) {
        bool length = k_token_spelled(name, "length"), capacity = k_token_spelled(name, "capacity"),
             dim = k_token_spelled(name, "dim");
        if (!(length || capacity || dim) || !argc) return -1;
        if (!k0 || k0->kind != KT_ARRAY) {
            if ((k0 && k0->kind != KT_NONE) || isl_container_shape(a, args[0]) == SHAPE_C)
                isl_diag3(c, K_DIAG_NOT_A_CONTAINER_EXPRESSION, isl_tok(a, args[0].first), written, isl_range_text(a, args[0]), none);
            return -1;
        }
        long known;
        if (dim && argc == 2 && !isl_decimal_of(c, args[1], &known))
            isl_diag3(c, K_DIAG_NONCONSTANT_DIM_INDEX, isl_tok(a, args[1].first), isl_range_text(a, args[1]), none, none);
        Text t = { .ok = true };
        isl_put(&t, written.ptr, written.len);
        isl_put_str(&t, "/");
        isl_put_uint(&t, argc);
        isl_put_str(&t, " \xe2\x86\x92 core dim:1");
        isl_emit(c, K_ISLAND_CALL, callee, &t);
        return prim_type(c, "size_t");
    }

    keel_slice_char redirected = {0};
    /* A function over a protocol (spec §4.14, backend §5.19): the instance is
       named by the concrete type of the argument, `<prefix>_<name><arity>_<type>`.
       The island resolves the call as the concrete type's own verb — the one
       whose result is the function's associated return type, or the first verb
       that takes the receiver — to find the instance, the adaptation and the
       type of the result. Only the first parameter may be the protocol one. */
    bool proto_of = false;
    size_t redirect_arity = argc;
    const KAst *proto_home = NULL;
    Sig fsig = {0};
    if (argc) {
        Sig f = isl_find_verb(home, name, argc);
        if (f.found && f.n && f.p[0].is_protocol) {
            KProtocolDecl pd;
            if (!isl_protocol_decl(c, home, f.p[0].base, &pd, 0)) return -1;
            const KModule *own_module = NULL;
            if (k0 && k0->kind == KT_ARRAY) {
                if (k0->rank > 1) {
                    isl_diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, isl_tok(a, callee), written, isl_range_text(a, args[0]), k0->dims);
                    return -1;
                }
                own_module = isl_module_by_name(a->symbols, "keel.array");
            } else if (k0 && (k0->kind == KT_MODIFIER || k0->kind == KT_NAMED)) {
                own_module = k0->module;
            } else {
                return -1;
            }
            const KAst *own = own_module && own_module->ast ? own_module->ast
                            : k0->kind == KT_ARRAY ? NULL : a;   /* a type of this very file */
            Text missing = { .ok = true };
            if (!own || !isl_satisfies(c, own, k0->kind == KT_ARRAY ? (keel_slice_char){0} : k0->name, &pd, &missing, isl_tok(a, callee))) {
                isl_diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, isl_tok(a, callee), isl_range_text(a, args[0]), f.p[0].base,
                      own ? isl_keep(c, &missing) : k_diag_text("its verbs"));
                return -1;
            }
            const KProtocolVerb *v = isl_verb_for_return(home, f.fn, &pd);
            if (!v) return -1;
            proto_of = true;
            proto_home = home;
            fsig = f;
            module = own_module;
            home = own;
            redirected = v->name;
            redirect_arity = v->arity;
        }
    }
    Sig sig = isl_find_verb(home, redirected.len ? redirected : name, redirected.len ? redirect_arity : argc);
    keel_slice_char declared = redirected.len ? redirected : name;
    bool gen = module && isl_generic(home);
    keel_slice_char modifier = gen ? isl_modifier_of(home) : none;

    /* which parameter takes the container, which one mentions the module's modifier
       inside another type, which takes an `array`, and whether a `type` parameter
       selects the instance (spec §4.4, item 4) */
    size_t recv = SIZE_MAX, nested = SIZE_MAX, arr = SIZE_MAX;
    bool selection = false;
    for (size_t k = 0; sig.found && k < sig.n; k++) {
        if (sig.p[k].is_type) selection = selection || isl_binder(home, sig.p[k].base);
        else if (sig.p[k].is_array) { if (arr == SIZE_MAX) arr = k; }
        else if (gen && recv == SIZE_MAX && k_symtab_same_name(sig.p[k].base, modifier)) recv = k;
        else if (gen && nested == SIZE_MAX && mentions_own(home, &sig.p[k])) nested = k;
    }
    if (!sig.found && gen && argc) recv = 0;        /* the module declares no such verb: the object is first */

    /* slice.from over a `ref` would take a pointer keel does not vouch for (spec §5.3) */
    if (module && isl_module_named(home, "keel.slice") && k_token_spelled(name, "from") && argc >= 2) {
        const Local *p = isl_arg_local(c, args[1]);
        if (p && p->is_ref)
            isl_diag3(c, K_DIAG_SLICE_FROM_REF, isl_tok(a, args[1].first), isl_range_text(a, args[1]), none, none);
    }

    Text t = { .ok = true };
    isl_put(&t, written.ptr, written.len);
    isl_put_str(&t, "/");
    isl_put_uint(&t, argc);
    isl_put_str(&t, " \xe2\x86\x92 ");

    /* buffer.of(x) needs an `array` of one dimension, to read the extent from (spec §5.3) */
    if (module && !redirected.len && isl_module_named(home, "keel.buffer") && k_token_spelled(name, "of") &&
        argc == 1 && k0 && k0->kind != KT_ARRAY && k0->kind != KT_NONE) {
        isl_diag3(c, K_DIAG_BUFFER_OF_UNKNOWN_SIZE, isl_tok(a, callee), written, isl_range_text(a, args[0]), none);
        return -1;
    }

    /* arena.from_array(a, v) takes an `array u8` by its binder (spec §5.2); the
       element type is the C compiler's to check */
    if (module && isl_module_named(home, "keel.arena") && k_token_spelled(name, "from_array") && argc == 2) {
        int t1 = isl_type_of(c, args[1]);
        if (t1 < 0 || isl_kt(c, t1)->kind != KT_ARRAY) return -1;
        Text array_form = { .ok = true };
        isl_put(&array_form, written.ptr, written.len);
        isl_put_str(&array_form, "/2 \xe2\x86\x92 ");
        isl_put_module_prefix(&array_form, home);
        isl_put_str(&array_form, "_from_array");
        if (k0 && k0->pointers == 0 && k0->lvalue) isl_put_str(&array_form, " &1");
        isl_put_str(&array_form, " dim:2");
        isl_emit(c, K_ISLAND_CALL, callee, &array_form);
        isl_roles_at_call(c, home, &sig, args, argc, callee, name);
        return prim_type(c, "bool");
    }

    /* `buffer.of(v)` over an `array` lowers to the buffer's `of`, with dimension 0
       from the table (backend §5.2) */
    if (gen && !redirected.len && k0 && k0->kind == KT_ARRAY && k_token_spelled(name, "of") &&
        isl_module_named(home, "keel.buffer")) {
        const char *elem = isl_kt_symbol(c, k0->elem);
        if (!*elem) return -1;
        if (k0->rank > 1) {
            isl_diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, isl_tok(a, callee), written, isl_range_text(a, args[0]), k0->dims);
            return -1;
        }
        if (argc == 1 && !strncmp(elem, "const_", 6))
            isl_diag3(c, K_DIAG_BUFFER_OVER_CONST, isl_tok(a, callee), written, (keel_slice_char){ strlen(elem), (char *)elem }, none);
        if (argc != 1) return -1;                   /* buffer.of(p, n) is the ordinary verb */
        isl_put_module_prefix(&t, home);
        isl_put_str(&t, "_");
        isl_put_str(&t, elem);
        isl_put_str(&t, "_of dim:1");
        isl_emit(c, K_ISLAND_CALL, callee, &t);
        /* the result is a buffer over the element */
        int view = isl_kt_new(c);
        if (view < 0) return -1;
        isl_kt(c, view)->kind = KT_MODIFIER;
        isl_kt(c, view)->module = module;
        isl_kt(c, view)->argc = 1;
        isl_kt(c, view)->arg[0] = kt_copy(c, k0->elem);
        char symbol[K_SYMBOL_MAX];
        Text sy = { .ok = true };
        isl_put_module_prefix(&sy, home);
        isl_put_str(&sy, "_");
        isl_put_str(&sy, elem);
        if (sy.ok) { memcpy(symbol, sy.s, sy.n); isl_kt_set_symbol(isl_kt(c, view), symbol, sy.n); }
        return view;
    }

    int inst = -1;                                  /* the instance of the module the verb belongs to */
    bool direct = false;                            /* the object is the modifier itself */
    if (gen) {
        if (recv != SIZE_MAX || nested != SIZE_MAX || arr != SIZE_MAX) {
            size_t k = recv != SIZE_MAX ? recv : nested != SIZE_MAX ? nested : arr;
            if (k >= argc) return -1;
            Range r = args[k];
            if (isl_punct(a, r.first, "&") && recv != SIZE_MAX) {
                isl_diag3(c, K_DIAG_ADDRESS_IN_OBJECT_POSITION, isl_tok(a, r.first), written, isl_range_text(a, r), none);
                return -1;
            }
            int tk = k == 0 ? t0 : isl_type_of(c, r);
            const KType *ok = tk >= 0 ? isl_kt(c, tk) : NULL;
            bool wanted = ok && (arr != SIZE_MAX && recv == SIZE_MAX && nested == SIZE_MAX ? ok->kind == KT_ARRAY
                                                                                            : ok->kind == KT_MODIFIER);
            if (!wanted) {
                /* only what is certainly not a container is refused: a name seen
                   declared as something else, or a shape that is C. A name this pass
                   could not have seen declared is not refused. */
                if ((ok && ok->kind != KT_NONE) || isl_container_shape(a, r) == SHAPE_C)
                    isl_diag3(c, K_DIAG_NOT_A_CONTAINER_EXPRESSION, isl_tok(a, r.first), written, isl_range_text(a, r), none);
                return -1;
            }
            if (recv != SIZE_MAX) {
                if (ok->module != module) {
                    if (ok->module && !sig.producer)
                        isl_diag3(c, K_DIAG_WRONG_QUALIFIER, isl_tok(a, callee), written, isl_alias_of(a->symbols, ok->module),
                              isl_range_text(a, r));
                    return -1;
                }
                inst = tk;
                direct = true;
                /* get and set copy an element: an instance of a modifier is not copied by value */
                if ((k_token_spelled(name, "get") || k_token_spelled(name, "set")) && ok->argc &&
                    ok->arg[0] >= 0 && isl_kt(c, ok->arg[0])->kind == KT_MODIFIER)
                    isl_diag3(c, K_DIAG_ELEMENT_COPY_IN_GET_SET, isl_tok(a, callee), written,
                          (keel_slice_char){ strlen(isl_kt_symbol(c, ok->arg[0])), (char *)isl_kt_symbol(c, ok->arg[0]) }, none);
            } else if (nested != SIZE_MAX) {
                inst = kt_find(c, tk, module, 0);
                if (inst < 0) return -1;
            } else {
                /* the verbs of `keel.array` are the family of one element type */
                inst = isl_kt_new(c);
                if (inst < 0) return -1;
                isl_kt(c, inst)->kind = KT_MODIFIER;
                isl_kt(c, inst)->module = module;
                isl_kt(c, inst)->argc = 1;
                isl_kt(c, inst)->arg[0] = kt_copy(c, ok->elem);
            }
        } else if (selection) {
            /* the written types are the instance's arguments, in parameter order */
            Text spelled = { .ok = true };
            keel_slice_char alias = isl_alias_of(a->symbols, module);    /* qualified: `types` may be absent */
            isl_put(&spelled, alias.ptr, alias.len);
            isl_put_str(&spelled, ".");
            isl_put(&spelled, modifier.ptr, modifier.len);
            for (size_t k = 0; k < sig.n && k < argc; k++) {
                if (!sig.p[k].is_type || !isl_binder(home, sig.p[k].base)) continue;
                isl_put_str(&spelled, " ");
                isl_put_tokens(&spelled, a, args[k].first, args[k].end);
            }
            if (!spelled.ok) return -1;
            inst = isl_kt_from_text(c, a->symbols, (keel_slice_char){ spelled.n, spelled.s });
        } else if (sig.found && c->has_target && c->target.kind == K_SPEC_MODIFIER) {
            inst = isl_kt_from_text(c, a->symbols, c->target.text);
            if (inst < 0 || isl_kt(c, inst)->module != module) return -1;
        } else {
            /* a verb that takes its instance from the target (spec §4.4, item 4):
               an assignment to a known symbol, or a return, gives one */
            KSpecifier spec;
            bool target = false;
            if (callee >= 2 && isl_punct(a, callee - 1, "=") && isl_ident(a, callee - 2)) {
                const Local *l = isl_find_local(c, isl_tok(a, callee - 2));
                if (l && l->spec.kind == K_SPEC_MODIFIER) { spec = l->spec; target = true; }
            } else if (callee >= 1 && isl_word(a, callee - 1, "return") && c->fn) {
                size_t s0 = c->fn->first;
                while (s0 < c->fn->name_first && (isl_word(a, s0, "pub") || isl_word(a, s0, "priv") || isl_word(a, s0, "inline") ||
                                                   isl_word(a, s0, "static") || k_token_role(isl_tok(a, s0)) != K_ROLE_NONE))
                    s0++;
                KLexer lexer;
                TKPpKind pp;
                KToken next = {0};
                KToken first = isl_enter(a, s0, &lexer, &pp);
                target = s0 < c->fn->name_first && k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) &&
                         spec.kind == K_SPEC_MODIFIER;
            }
            if (!target) {
                if (sig.found) {
                    Text written = { .ok = true };
                    isl_put(&written, isl_tok(a, callee).ptr, (size_t)(isl_tok(a, verb).ptr + isl_tok(a, verb).len - isl_tok(a, callee).ptr));
                    isl_diag3(c, K_DIAG_FROM_WITHOUT_TARGET, isl_tok(a, callee), isl_keep(c, &written), (keel_slice_char){0},
                          (keel_slice_char){0});
                }
                return -1;
            }
            inst = isl_kt_from_text(c, a->symbols, spec.text);
            if (inst < 0 || isl_kt(c, inst)->module != module) return -1;
        }
        if (inst < 0 || isl_kt(c, inst)->kind != KT_MODIFIER) return -1;
        {
            /* the instance the call takes, for the closure */
            Text own = { .ok = true };
            if (*isl_kt(c, inst)->symbol) isl_put_str(&own, isl_kt(c, inst)->symbol);
            else module_instance_prefix(c, home, inst, &own);
            if (own.ok && own.n < K_SYMBOL_MAX) {
                own.s[own.n] = 0;
                isl_record_seed(c, inst, own.s, K_CLOSURE_INSTANCE, module);
                if (proto_of) {
                    Text fn = { .ok = true };
                    isl_put_module_prefix(&fn, proto_home);
                    isl_put_str(&fn, "_");
                    isl_put(&fn, name.ptr, name.len);
                    isl_put_str(&fn, "_");
                    isl_put(&fn, own.s, own.n);
                    if (fn.ok && fn.n < K_SYMBOL_MAX) {
                        fn.s[fn.n] = 0;
                        isl_record_seed(c, -1, fn.s, K_CLOSURE_PROTOCOL_FUNCTION, ast_module(c, proto_home));
                    }
                }
            }
        }
        if (proto_of) {
            /* keel_slice_of<arity>_<concrete type> (backend §5.19): the concrete
               type's canonical name is the prefix its own verbs carry */
            Text own = { .ok = true };
            if (direct ? (isl_put_str(&own, isl_kt(c, inst)->symbol), !*isl_kt(c, inst)->symbol)
                       : !module_instance_prefix(c, home, inst, &own)) return -1;
            isl_put_module_prefix(&t, proto_home);
            isl_put_str(&t, "_");
            isl_put(&t, name.ptr, name.len);
            if (fsig.overloaded && argc > 1) isl_put_uint(&t, argc - 1);
            isl_put_str(&t, "_");
            isl_put(&t, own.s, own.n);
        } else if (direct) {
            if (!*isl_kt(c, inst)->symbol) return -1;
            isl_put_str(&t, isl_kt(c, inst)->symbol);
        } else if (!module_instance_prefix(c, home, inst, &t)) {
            return -1;
        }
        /* a verb the instance does not keep (spec §4.3, rule 13) */
        const char *cause = NULL;
        if (!proto_of && sig.found && !c->quiet && !isl_verb_available(c, home, sig.fn, inst, &cause)) {
            Text written = { .ok = true };
            isl_put(&written, isl_tok(a, callee).ptr, (size_t)(isl_tok(a, verb).ptr + isl_tok(a, verb).len - isl_tok(a, callee).ptr));
            Text where = { .ok = true }, why = { .ok = true };
            isl_put_str(&where, isl_kt(c, inst)->symbol);
            isl_put_str(&why, cause);
            isl_diag3(c, K_DIAG_VERB_NOT_IN_INSTANCE, isl_tok(a, callee), isl_keep(c, &written), isl_keep(c, &where), isl_keep(c, &why));
        }
    } else if (proto_of) {
        /* a type of a module that is not generic: its canonical name is its symbol */
        if (!k0 || !*k0->symbol) return -1;
        isl_put_module_prefix(&t, proto_home);
        isl_put_str(&t, "_");
        isl_put(&t, name.ptr, name.len);
        if (fsig.overloaded && argc > 1) isl_put_uint(&t, argc - 1);
        isl_put_str(&t, "_");
        isl_put_str(&t, k0->symbol);
    } else {
        isl_put_module_prefix(&t, home);
    }
    if (!proto_of) {
        isl_put_str(&t, "_");
        isl_put(&t, declared.ptr, declared.len);
        if (sig.found) {
            const KType *ki = inst >= 0 ? isl_kt(c, inst) : NULL;
            if (sig.rank_binder >= 0 && (!ki || !isl_put_verb_suffix(c, ki, &sig, argc, &t))) return -1;
            if (sig.rank_binder < 0 && sig.overloaded && argc > 1) isl_put_uint(&t, argc - 1);
        }
    }

    /* a function over a protocol has its own roles, not those of the verb it
       resolves through */
    if (proto_of) isl_roles_at_call(c, proto_home, &fsig, args, argc, callee, name);
    else isl_roles_at_call(c, home, &sig, args, argc, callee, name);

    /* a value of a set where the verb's parameter is the module's `tags`
       binder belongs to the instance's set (spec §4.3, rule 6) */
    if (inst >= 0 && sig.found && !proto_of && isl_kt(c, inst)->kind == KT_MODIFIER) {
        const KAstNode *mn = isl_module_node(home);
        size_t bi = 0;
        for (size_t j = mn->tags_first; j < mn->tags_end; j++) {
            if (!isl_ident(home, j)) continue;
            for (size_t k = 0; k < argc && k < sig.n; k++) {
                if (!k_symtab_same_name(sig.p[k].base, isl_tok(home, j)) || sig.p[k].pointer) continue;
                if (args[k].end != args[k].first + 1 || !isl_ident(a, args[k].first) || (int)bi >= isl_kt(c, inst)->argc) continue;
                const KType *set = isl_kt(c, isl_kt(c, inst)->arg[bi]);
                const KAst *where = set->module && isl_ast_of(c, set->module) ? isl_ast_of(c, set->module) : a;
                KTagsDecl tags;
                if (!isl_tags_decl(where, set->name, &tags)) continue;
                bool in = false;
                for (size_t m = 0; m < tags.item_count; m++) if (k_symtab_same_name(tags.items[m], isl_tok(a, args[k].first))) in = true;
                if (!in && isl_tag_of_some_set(a, isl_tok(a, args[k].first)))
                    isl_diag3(c, K_DIAG_TAG_FROM_OTHER_SET, isl_tok(a, args[k].first), isl_tok(a, args[k].first), set->name,
                          (keel_slice_char){0});
            }
            bi++;
        }
    }

    /* adaptation marks, from the declared parameter (spec §4.4, item 5) */
    for (size_t k = 0; sig.found && k < argc && k < sig.n; k++) {
        if (sig.p[k].is_type) {
            if (!isl_binder(home, sig.p[k].base)) { isl_put_str(&t, " type:"); isl_put_uint(&t, k + 1); }
            continue;
        }
        int tk = k == 0 ? t0 : isl_type_of(c, args[k]);
        const KType *ak = tk >= 0 ? isl_kt(c, tk) : NULL;
        if (sig.p[k].is_array) {
            if (ak && ak->kind == KT_ARRAY) { isl_put_str(&t, " dim:"); isl_put_uint(&t, k + 1); }
            check_array_argument(c, home, &sig.p[k], args[k], ak);
            continue;
        }
        if (sig.p[k].pointer && ak && ak->pointers == 0 && ak->lvalue) { isl_put_str(&t, " &"); isl_put_uint(&t, k + 1); }
    }
    isl_emit(c, K_ISLAND_CALL, callee, &t);
    if (proto_of && !isl_returns_associated(proto_home, fsig.fn))
        return isl_ret_type(c, proto_home, fsig.fn, -1);
    return sig.found && sig.fn ? isl_ret_type(c, home, sig.fn, inst) : -1;
}

/* a call at token `i`, if it is one */
bool isl_call_at(Ctx *c, size_t i) {
    Callee ce;
    isl_resolution_begin(c);
    bool is = callee_at(c, i, &ce);
    if (is) call_core(c, i, ce.verb, ce.open, ce.module);
    isl_resolution_end(c);
    return is;
}

/* ---- names ------------------------------------------------------------- */

/* the tags item `name`, if some `tags` declaration of the file lists it: its symbol */
static bool tag_value(const KAst *a, keel_slice_char name, Text *t) {
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind != K_AST_TAGS) continue;
        size_t j = n->name_end;
        if (!isl_punct(a, j, "[")) continue;
        for (bool item = true; j < n->end && !isl_punct(a, j, "]"); j++) {
            if (isl_punct(a, j, "[") || isl_punct(a, j, ",")) item = true;
            else if (item && isl_ident(a, j)) {
                item = false;
                if (!k_symtab_same_name(isl_tok(a, j), name)) continue;
                isl_put_module_prefix(t, a);
                isl_put_str(t, "_");
                isl_put_glued(t, a, n->name_first, n->name_end);
                isl_put_str(t, "_");
                isl_put(t, name.ptr, name.len);
                return true;
            }
        }
    }
    return false;
}

/* whether `name` is a value of a set declared in `where` */
bool isl_tag_of_some_set(const KAst *where, keel_slice_char name) {
    Text scratch = { .ok = true };
    return tag_value(where, name, &scratch);
}

void isl_name_island(Ctx *c, size_t i) {
    KAst *a = c->ast;
    KToken name = isl_tok(a, i);
    const Local *l = isl_find_local(c, name);
    if (l && !l->global) return;                    /* a local, or a parameter: not a name of the module */
    Text t = { .ok = true };
    isl_put(&t, name.ptr, name.len);
    isl_put_str(&t, " \xe2\x86\x92 ");
    const KSymbol *s = k_symbol_resolve(a->symbols, name);
    if (s && (s->kind == K_SYM_CONSTANT || s->kind == K_SYM_VARIABLE || s->kind == K_SYM_FUNCTION) && s->origin) {
        char sym[K_DETAIL_MAX];
        size_t n = k_mangle_symbol(s, (keel_slice_char){ sizeof sym, sym });
        if (n > sizeof sym) return;
        isl_put(&t, sym, n);
    } else if (!tag_value(a, name, &t)) {
        return;
    }
    isl_emit(c, K_ISLAND_NAME, i, &t);
}

/* `Q.IDENT` that is not a call: a constant, a variable or a function of the
   module the qualifier names. What the module does not export goes on as the
   qualified name, for the C compiler to validate (spec §4.4, item 3). */
void isl_qualified_name(Ctx *c, size_t i, const KModule *module) {
    KAst *a = c->ast;
    KToken member = isl_tok(a, i + 2);
    Text t = { .ok = true };
    isl_put_glued(&t, a, i, i + 3);
    isl_put_str(&t, " \xe2\x86\x92 ");
    const KSymbol *found = NULL;
    for (size_t k = 0; k < module->symbol_count && !found; k++)
        if (k_symtab_same_name(module->symbols[k].name, member)) found = &module->symbols[k];
    if (found) {
        char sym[K_DETAIL_MAX];
        size_t n = k_mangle_symbol(found, (keel_slice_char){ sizeof sym, sym });
        if (n > sizeof sym) return;
        isl_put(&t, sym, n);
    } else if (module->ast) {
        /* a value of a set of the module, written without the level of the set:
           from outside, `M.Set.VALUE` (spec §4.2, item 9) */
        if (isl_tag_of_some_set(module->ast, member)) {
            Text written = { .ok = true };
            isl_put_glued(&written, a, i, i + 3);
            isl_diag3(c, K_DIAG_ENUM_CONSTANT_WITHOUT_TYPE, isl_tok(a, i), isl_keep(c, &written), member, (keel_slice_char){0});
            return;
        }
        isl_put_module_prefix(&t, module->ast);
        isl_put_str(&t, "_");
        isl_put(&t, member.ptr, member.len);
    } else {
        return;
    }
    isl_emit(c, K_ISLAND_NAME, i, &t);
}

/* ---- declarations ------------------------------------------------------ */

bool isl_is_arena(const KAst *a, const KSpecifier *spec) {
    if (spec->kind != K_SPEC_NAMED_TYPE) return false;
    const KSymbol *s = k_symbol_resolve(a->symbols, spec->type_name);
    return s && s->kind == K_SYM_TYPE && s->origin &&
           k_symtab_same_name(s->origin->name, k_diag_text("keel.arena")) &&
           k_symtab_same_name(s->name, k_diag_text("arena"));
}

/* A keel type at token `i`: its `type` island, then what it declares. Returns
   the index of the last token of the specifier. */


/* a name the body of a `later` defer reads, declared again inside: the
   cleanup at an exit from here would read this one (defer-later-shadowed) */
void isl_note_shadow(Ctx *c, KToken name) {
    KAst *a = c->ast;
    for (size_t k = 0; k < c->later_count; k++) {
        if (c->depth <= c->later[k].depth || c->later[k].shadow.len) continue;
        for (size_t j = c->later[k].body_first; j < c->later[k].body_end; j++)
            if (isl_ident(a, j) && k_symtab_same_name(isl_tok(a, j), name)) {
                c->later[k].shadow = name;
                c->later[k].shadow_depth = c->depth;
                break;
            }
    }
}

/* whether `name` is the binder of an `array` parameter of `fn`: `[size_t name]` */
bool isl_binder_of(const KAst *a, const KAstNode *fn, KToken name) {
    size_t end = fn->body_first != SIZE_MAX ? fn->body_first : fn->end;
    for (size_t j = fn->name_end; j + 3 < end; j++)
        if (isl_punct(a, j, "[") && isl_word(a, j + 1, "size_t") && k_symtab_same_name(isl_tok(a, j + 2), name) && isl_punct(a, j + 3, "]"))
            return true;
    return false;
}

/* `int fd = 3;`, `struct S *p;`: a C declaration opening a statement, read
   only far enough to find the name it declares */
bool isl_c_declares(const KAst *a, size_t i, size_t from) {
    if (!(isl_punct(a, i + 1, "=") || isl_punct(a, i + 1, ";") || isl_punct(a, i + 1, ",") || isl_punct(a, i + 1, "["))) return false;
    size_t j = i, types = 0;
    while (j > from && j + 6 > i) {
        size_t p = j - 1;
        if (isl_punct(a, p, ";") || isl_punct(a, p, "{") || isl_punct(a, p, "}")) return types > 0;
        if (isl_punct(a, p, "*")) { j = p; continue; }
        if (isl_word(a, p, "return") || isl_word(a, p, "goto") || isl_word(a, p, "case") || isl_word(a, p, "else") ||
            isl_word(a, p, "do") || isl_word(a, p, "sizeof"))
            return false;                                   /* a statement, not a declaration */
        if (isl_ident(a, p) || k_token_is_c_word(isl_tok(a, p))) { types++; j = p; continue; }
        return false;
    }
    return false;
}

size_t isl_declaration(Ctx *c, size_t i, const KSpecifier *spec, KLexer *lexer, KToken next,
                          bool signature, bool declarators) {
    KAst *a = c->ast;
    isl_check_arguments(c, spec, isl_tok(a, i));
    size_t end = isl_index_at(a, i, spec->text.ptr + spec->text.len);

    bool primitive = spec->kind == K_SPEC_NAMED_TYPE && k_primitive_name(spec->type_name);
    if (!primitive) {
        char sym[K_DETAIL_MAX];
        size_t n = SIZE_MAX;
        if (spec->kind == K_SPEC_MODIFIER) n = k_spec_symbol(spec->text, a->symbols, sym, sizeof sym);
        else {
            const KSymbol *s = k_symbol_resolve(a->symbols, spec->type_name);
            if (s && s->origin) n = k_mangle_symbol(s, (keel_slice_char){ sizeof sym, sym });
        }
        if (n <= sizeof sym) {
            Text t = { .ok = true };
            isl_put_tokens(&t, a, i, end);
            isl_put_str(&t, " \xe2\x86\x92 ");
            isl_put(&t, sym, n);
            isl_emit(c, K_ISLAND_TYPE, i, &t);
        }
    }

    if (!declarators) return end - 1;

    bool external = i > 0 && isl_word(a, i - 1, "extern");
    TKPpKind pp;
    for (KToken d = next; d.len; ) {
        KDeclarator decl;
        KToken after;
        if (!(k_token_is_punct(d, "*") || k_token_is_punct(d, "(") || k_token_is_ident(d))) break;
        if (!k_scan_declarator(lexer, d, &decl, &after, &pp) || !decl.name.len) break;
        if (decl.has_function_suffix && !decl.parenthesized) break;   /* a prototype */
        size_t at = isl_index_at(a, i, decl.name.ptr);
        if (!signature && c->depth > 0) isl_check_name(c, decl.name, false, false);
        if (!signature) isl_note_shadow(c, decl.name);
        isl_add_local(c, (Local){ .name = decl.name, .spec = *spec, .decl_at = at,
                              .pointers = decl.pointer_depth + (decl.has_array_suffix ? 1 : 0) });

        bool initialized = k_token_is_punct(after, "=");
        if (!signature && !external && !initialized && decl.pointer_depth == 0 &&
            !decl.parenthesized && isl_is_arena(a, spec) &&
            (k_token_is_punct(after, ",") || k_token_is_punct(after, ";"))) {
            Text t = { .ok = true };
            isl_put(&t, decl.name.ptr, decl.name.len);
            isl_emit(c, K_ISLAND_IMPLICIT_INIT, at, &t);
        }
        if (initialized) {
            static const char *const stops[] = { ",", ";" };
            size_t which;
            if (!c->has_target) { c->has_target = true; c->target = *spec; }
            KToken value = k_lexer_next(lexer, &pp);
            k_scan_opaque_until(lexer, value, stops, 2, &which, &after, &pp);
        }
        /* in `(T *e, M.cursor c : x)` the comma separates binders, not declarators */
        bool binder = i > 0 && (isl_punct(a, i - 1, "(") || isl_punct(a, i - 1, ","));
        if (signature || binder || !k_token_is_punct(after, ",")) break;
        d = k_lexer_next(lexer, &pp);
    }
    return end - 1;
}

bool isl_c_declares(const KAst *a, size_t i, size_t from);
bool isl_binder_of(const KAst *a, const KAstNode *fn, KToken name);
