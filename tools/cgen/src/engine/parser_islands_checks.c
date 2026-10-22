/* Pass 3: the names across modules, the declarations of passage 2, the protocol declarations, the parameters and the layout. Split from parser_islands.c by T3; the code is unchanged. */

#include "engine/parser_islands_internal.h"

void isl_check_arguments(Ctx *c, const KSpecifier *spec, KToken at);
void isl_check_name(Ctx *c, KToken name, bool top, bool function);
void isl_check_constexpr(Ctx *c, size_t kw, size_t end);
bool isl_spec_byref(const KAst *a, const KSpecifier *spec);
/* ---- names across modules (spec §2.5, §4.1) ---------------------------- */

static bool module_generic(const KModule *m) {
    if (!m || !m->ast) return false;
    const KAstNode *n = isl_node_at(m->ast, m->ast->module);
    return n->dim_first != n->type_end;
}

/* the canonical names a symbol takes in C: its own, and each value of a set */
static size_t canonical_names(const KSymbol *sym, const KAst *own, char out[][K_SYMBOL_MAX], size_t cap) {
    char buf[K_SYMBOL_MAX];
    size_t n = k_mangle_symbol(sym, (keel_slice_char){ sizeof buf, buf });
    if (n >= sizeof buf || !cap) return 0;
    memcpy(out[0], buf, n);
    out[0][n] = 0;
    size_t count = 1;
    KTagsDecl tags;
    const KAst *home = sym->origin && sym->origin->ast ? sym->origin->ast : own;    /* the file's own: not retained yet */
    if (sym->kind == K_SYM_TAGS && home && isl_tags_decl(home, sym->name, &tags))
        for (size_t k = 0; k < tags.item_count && count < cap; k++) {
            if (n + 1 + tags.items[k].len >= K_SYMBOL_MAX) continue;
            memcpy(out[count], buf, n);
            out[count][n] = '_';
            memcpy(out[count] + n + 1, tags.items[k].ptr, tags.items[k].len);
            out[count][n + 1 + tags.items[k].len] = 0;
            count++;
        }
    return count;
}

/* Spec §4.1, rule 9: the module's exported symbols against those of the
   transitive closure of its imports, by canonical name (symbol-collision). */
void isl_check_collisions(Ctx *c) {
    KAst *a = c->ast;
    const KAstNode *mod = isl_module_node(a);
    keel_slice_char self = isl_range_text(a, (Range){ mod->name_first, mod->name_end });
    const KModule *seen[64];
    size_t ns = 0;
    for (size_t i = 0; i < a->symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(a->symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin && ns < 64) {
            bool dup = false;
            for (size_t k = 0; k < ns; k++) if (seen[k] == s->origin) dup = true;
            if (!dup) seen[ns++] = s->origin;
        }
    }
    for (size_t q = 0; q < ns; q++) {                          /* the closure */
        const KAst *ma = seen[q]->ast;
        if (!ma || !ma->symbols) continue;
        for (size_t i = 0; i < ma->symbols->len && ns < 64; i++) {
            const KSymbol *s = keel_buffer_KSymbol_ptr(ma->symbols, i);
            if (s->kind != K_SYM_MODULE || !s->origin) continue;
            bool dup = false;
            for (size_t k = 0; k < ns; k++) if (seen[k] == s->origin) dup = true;
            if (!dup) seen[ns++] = s->origin;
        }
    }
    for (size_t i = 0; i < a->symbols->len; i++) {
        const KSymbol *own = keel_buffer_KSymbol_ptr(a->symbols, i);
        if (!own->origin || !k_symtab_same_name(own->origin->name, self) || own->kind == K_SYM_MODULE ||
            own->kind == K_SYM_MODIFIER || own->kind == K_SYM_PROTOCOL)
            continue;
        char mine[K_TAGS_MAX_ITEMS + 1][K_SYMBOL_MAX];
        size_t nm = canonical_names(own, a, mine, K_TAGS_MAX_ITEMS + 1);
        for (size_t q = 0; q < ns; q++) {
            const KModule *m = seen[q];
            if (module_generic(m) || k_symtab_same_name(m->name, self)) continue;
            for (size_t k = 0; k < m->symbol_count; k++) {
                const KSymbol *other = &m->symbols[k];
                if (other->kind == K_SYM_MODIFIER || other->kind == K_SYM_PROTOCOL) continue;
                char theirs[K_TAGS_MAX_ITEMS + 1][K_SYMBOL_MAX];
                size_t nt = canonical_names(other, NULL, theirs, K_TAGS_MAX_ITEMS + 1);
                for (size_t x = 0; x < nm; x++)
                    for (size_t y = 0; y < nt; y++)
                        if (!strcmp(mine[x], theirs[y])) {
                            Text name = { .ok = true };
                            isl_put_str(&name, mine[x]);
                            KToken at = own->name;
                            for (size_t j = 0; j < a->nodes.len; j++) {
                                const KAstNode *d = isl_node_at(a, j);
                                if (d->name_first != d->name_end && k_symtab_same_name(isl_tok(a, d->name_first), own->name))
                                    { at = isl_tok(a, d->name_first); break; }
                            }
                            isl_diag3(c, K_DIAG_SYMBOL_COLLISION, at, isl_keep(c, &name), own->name, m->name);
                        }
            }
        }
    }
}

/* Spec §2.5: an alias of a module and a type of another origin spelled
   alike in the file (alias-type-collision). The alias `outcome` and the
   modifier `outcome` that `types` injects share an origin, and are fine. */
void isl_check_alias_types(Ctx *c) {
    KAst *a = c->ast;
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind != K_AST_IMPORT || n->alias == SIZE_MAX) continue;
        KToken alias = isl_tok(a, n->alias);
        const KSymbol *m = NULL;
        for (size_t k = 0; k < a->symbols->len; k++) {
            const KSymbol *s = keel_buffer_KSymbol_ptr(a->symbols, k);
            if (s->kind == K_SYM_MODULE && k_symtab_same_name(s->name, alias)) { m = s; break; }
        }
        if (!m) continue;
        for (size_t k = 0; k < a->symbols->len; k++) {
            const KSymbol *s = keel_buffer_KSymbol_ptr(a->symbols, k);
            bool typeish = s->kind == K_SYM_TYPE || s->kind == K_SYM_MODIFIER || s->kind == K_SYM_TAGS ||
                           s->kind == K_SYM_PROTOCOL;
            if (typeish && s->origin != m->origin && k_symtab_same_name(s->name, alias)) {
                isl_diag3(c, K_DIAG_ALIAS_TYPE_COLLISION, alias, alias, s->origin ? s->origin->name : k_diag_text("this file"),
                      m->origin ? m->origin->name : alias);
                break;
            }
        }
    }
}

/* ---- declarations (passage 2: spec §2.5, §4.1, §4.2, §4.3, §4.9) ------- */

/* a declared name: in the reserved spaces (reserved-name), a contextual word
   of keel or a qualifier of an import (keel-name-shadowed, a warning only:
   where the name stands as a construction, keel reads the construction), or a
   name `types` injected (shadowed-injected-name) */
void isl_check_name(Ctx *c, KToken name, bool top, bool function) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (k_token_starts_with(name, "keel_") || k_token_starts_with(name, "KEEL_"))
        isl_diag3(c, K_DIAG_RESERVED_NAME, name, name, none, none);
    /* a contextual word, roles aside (they are not in the list); a function
       named `win` or `fail` is a verb, always written qualified, and shadows
       nothing (spec §2.5) */
    bool verb = function && (k_token_spelled(name, "win") || k_token_spelled(name, "fail"));
    bool shadows = k_token_is_keel_word(name) && !verb;
    /* or a qualifier of an import of the file */
    for (size_t k = 0; !shadows && k < a->symbols->len; k++) {
        const KSymbol *q = keel_buffer_KSymbol_ptr(a->symbols, k);
        if (q->kind == K_SYM_MODULE && k_symtab_same_name(q->name, name)) shadows = true;
    }
    if (shadows) isl_diag3(c, K_DIAG_KEEL_NAME_SHADOWED, name, name, none, none);
    /* a name `types` injected from another module, declared again here: the
       bare name now means this declaration (shadowed-injected-name) */
    const KAstNode *mod = isl_module_node(a);
    keel_slice_char self = isl_range_text(a, (Range){ mod->name_first, mod->name_end });
    for (size_t k = 0; k < a->symbols->len; k++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(a->symbols, k);
        if ((s->kind == K_SYM_TYPE || s->kind == K_SYM_MODIFIER || s->kind == K_SYM_PROTOCOL) && s->origin &&
            !k_symtab_same_name(s->origin->name, self) && k_symtab_same_name(s->name, name)) {
            isl_diag3(c, K_DIAG_SHADOWED_INJECTED_NAME, name, name, s->origin->name, none);
            break;
        }
    }
    (void)top;
}

/* whether the type a specifier names is `byref`: a modifier declared
   `modifier X byref`, a `typedef ... X byref;`, or `arena` (spec §4.3, rule 17) */
bool isl_spec_byref(const KAst *a, const KSpecifier *spec) {
    if (isl_is_arena(a, spec)) return true;
    keel_slice_char name = spec->kind == K_SPEC_MODIFIER ? spec->modifier_name : spec->type_name;
    const KSymbol *sym = k_symbol_resolve(a->symbols, name);
    const KAst *home = sym && sym->origin && sym->origin->ast ? sym->origin->ast : a;
    if (!sym) return false;
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *n = isl_node_at(home, i);
        if (n->name_first == n->name_end || !k_symtab_same_name(isl_tok(home, n->name_first), sym->name)) continue;
        if (n->kind == K_AST_MODIFIER) return isl_word(home, n->name_end, "byref");
        if (n->kind == K_AST_TYPE)
            for (size_t j = n->name_end; j < n->end; j++) if (isl_word(home, j, "byref")) return true;
    }
    return false;
}

/* the dimensions and the `tags` arguments of a modifier written in `spec`
   (spec §4.3, rules 4 and 5): a `dim` is a known decimal of at least 1
   (nonconstant-dim, dim-below-one), and a `tags` argument names a set
   (undeclared-tags) */
void isl_check_arguments(Ctx *c, const KSpecifier *spec, KToken at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (spec->kind != K_SPEC_MODIFIER) return;
    for (size_t d = 0; d < spec->dim_count; d++) {
        keel_slice_char t = spec->dims[d];
        while (t.len && t.ptr[0] == ' ') { t.ptr++; t.len--; }
        while (t.len && t.ptr[t.len - 1] == ' ') t.len--;
        long v = -1;
        bool known = isl_digits(t, &v);
        if (!known) {
            const Local *l = isl_find_local(c, t);
            const KSymbol *sym = l ? NULL : k_symbol_resolve(a->symbols, t);
            known = l ? l->is_constexpr && isl_digits(l->value, &v) : sym && sym->kind == K_SYM_CONSTANT && isl_digits(sym->value, &v);
        }
        if (!known) isl_diag3(c, K_DIAG_NONCONSTANT_DIM, at, t, spec->text, none);
        else if (v < 1) isl_diag3(c, K_DIAG_DIM_BELOW_ONE, at, t, spec->text, none);
    }
    const KSymbol *m = k_symbol_resolve(a->symbols, spec->modifier_name);
    const KAst *home = m && m->origin ? m->origin->ast : NULL;
    if (!home) return;
    const KAstNode *mn = isl_module_node(home);
    size_t tags = 0;
    for (size_t j = mn->tags_first; j < mn->tags_end; j++) if (isl_ident(home, j)) tags++;
    for (size_t k = 0; k < tags && k < spec->arg_count; k++) {
        const KSymbol *s = k_symbol_resolve(a->symbols, spec->args[k]);
        if (!s || s->kind != K_SYM_TAGS) isl_diag3(c, K_DIAG_UNDECLARED_TAGS, at, spec->args[k], spec->text, none);
    }
}

static void check_type_params(Ctx *c, const KAstNode *fn);
static void check_protocol_body(Ctx *c, const KAstNode *fn);

void isl_check_declarations(Ctx *c) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    const KAstNode *mod = isl_module_node(a);
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        bool named = n->name_first != n->name_end;
        /* visibility and storage (spec §4.1) */
        size_t name_at = named ? n->name_first : n->end;
        bool has_static = false, has_inline = false, has_pub = isl_word(a, n->first, "pub");
        bool has_typedef = false;
        for (size_t j = n->first; j < name_at && j < n->end; j++) {
            if (isl_word(a, j, "static")) has_static = true;
            if (isl_word(a, j, "inline")) has_inline = true;
            if (isl_word(a, j, "typedef")) has_typedef = true;
        }
        if (n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE) {
            if (has_pub && has_static && !has_inline) isl_diag3(c, K_DIAG_PUB_STATIC, isl_tok(a, n->first), none, none, none);
            if (isl_word(a, n->first, "static") && has_inline) isl_diag3(c, K_DIAG_INLINE_WITHOUT_VISIBILITY, isl_tok(a, n->first), none, none, none);
        }
        if ((n->kind == K_AST_TYPE || has_typedef) && has_static) isl_diag3(c, K_DIAG_STATIC_ON_TYPE, isl_tok(a, n->first), none, none, none);
        /* sets of tags (spec §4.9) */
        if (n->kind == K_AST_TAGS) {
            if (!named) { isl_diag3(c, K_DIAG_UNNAMED_TAGS, isl_tok(a, n->first), none, none, none); continue; }
            KTagsDecl t;
            if (isl_tags_decl(a, isl_tok(a, n->name_first), &t)) {
                if (!t.item_count) isl_diag3(c, K_DIAG_EMPTY_TAGS, isl_tok(a, n->name_first), isl_tok(a, n->name_first), none, none);
                if (t.has_values && !t.all_values)
                    isl_diag3(c, K_DIAG_PARTIAL_TAG_VALUES, isl_tok(a, n->name_first), isl_tok(a, n->name_first), none, none);
                for (size_t k = 0; k < t.item_count; k++) {
                    long v;
                    keel_slice_char val = t.values[k];
                    if (!val.len) continue;
                    if (val.ptr[0] == '-') { val.ptr++; val.len--; while (val.len && val.ptr[0] == ' ') { val.ptr++; val.len--; } }
                    const KSymbol *sym = isl_digits(val, &v) ? NULL : k_symbol_resolve(a->symbols, val);
                    if (!isl_digits(val, &v) && !(sym && sym->kind == K_SYM_CONSTANT))
                        isl_diag3(c, K_DIAG_NONCONSTANT_TAG_VALUE, t.items[k], t.values[k], t.items[k], none);
                }
            }
            for (size_t k = 0; k < i; k++) {
                const KAstNode *o = isl_node_at(a, k);
                if (o->kind == K_AST_TAGS && o->name_first != o->name_end &&
                    k_symtab_same_name(isl_tok(a, o->name_first), isl_tok(a, n->name_first))) {
                    isl_diag3(c, K_DIAG_DUPLICATE_TAGS_NAME, isl_tok(a, n->name_first), isl_tok(a, n->name_first), none, none);
                    break;
                }
            }
        }
        if (!named || n->kind == K_AST_IMPORT || n->kind == K_AST_IMPORT_C || n->kind == K_AST_MODULE ||
            n->kind == K_AST_INSTANCE)
            continue;
        KToken name = isl_tok(a, n->name_first);
        isl_check_name(c, name, true, n->kind == K_AST_FUNCTION);
        /* the module's parameters are not redeclared (spec §4.3) */
        for (size_t k = mod->dim_first; k < mod->type_end; k++)
            if (isl_ident(a, k) && k_symtab_same_name(isl_tok(a, k), name))
                isl_diag3(c, K_DIAG_PARAMETER_NAME_REUSE, name, name, none, none);
        /* a function with a `defer` returns through a temporary of the written
           return type, which the tokens before the name have to spell: not so
           when the name sits inside the declarator, `int (*f(void))[10]` */
        if (n->body_first != SIZE_MAX &&
            (isl_punct(a, n->name_first - 1, "(") || (isl_punct(a, n->name_first - 1, "*") && isl_punct(a, n->name_first - 2, "(")))) {
            bool deferred = false, valued = false;
            for (size_t j = n->body_first; j < n->body_end; j++) {
                if (isl_word(a, j, "defer")) deferred = true;
                if (isl_word(a, j, "return") && !isl_punct(a, j + 1, ";")) valued = true;
            }
            if (deferred && valued) isl_diag3(c, K_DIAG_HIDDEN_DECLARATOR, isl_tok(a, n->name_first), isl_tok(a, n->name_first), none, none);
        }
        if (n->kind != K_AST_FUNCTION) continue;
        check_type_params(c, n);
        /* a binder of dimension 0 is not a dimension of a vector (spec §4.2, rule 20) */
        for (size_t j = n->body_first; n->body_first != SIZE_MAX && j + 1 < n->body_end; j++)
            if (isl_ident(a, j) && isl_punct(a, j - 1, "[") && isl_punct(a, j + 1, "]") && isl_ident(a, j - 2) &&
                isl_c_declares(a, j - 2, n->body_first - 1) && isl_binder_of(a, n, isl_tok(a, j)))
                isl_diag3(c, K_DIAG_BINDER_AS_DIMENSION, isl_tok(a, j), isl_tok(a, j), none, none);
        check_protocol_body(c, n);
        Param p[K_PARAMS_MAX];
        size_t np = isl_params_of(a, n, p, K_PARAMS_MAX);
        for (size_t k = 0; np != SIZE_MAX && k < np; k++) {
            if (p[k].end <= p[k].first) continue;
            KToken pn = isl_tok(a, p[k].end - 1);
            if (isl_ident(a, p[k].end - 1) && !p[k].is_type) {      /* `type T` selects the instance (§4.4) */
                isl_check_name(c, pn, false, false);
                for (size_t m = mod->dim_first; m < mod->type_end; m++)
                    if (isl_ident(a, m) && k_symtab_same_name(isl_tok(a, m), pn))
                        isl_diag3(c, K_DIAG_PARAMETER_NAME_REUSE, pn, pn, none, none);
            }
            /* a `byref` instance by value (spec §4.3, rule 17) */
            Range r = isl_param_type(a, &p[k]);
            if (p[k].pointer || p[k].is_type || p[k].is_array || r.end <= r.first) continue;
            KLexer lexer;
            TKPpKind pp;
            KSpecifier spec;
            KToken next = {0};
            KToken first = isl_enter(a, r.first, &lexer, &pp);
            if (k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE &&
                isl_spec_byref(a, &spec))
                isl_diag3(c, K_DIAG_BYREF_PARAM, pn, pn, spec.text, none);
        }
    }
    /* the canonical names of the module's own declarations (spec §4.2) */
    keel_slice_char self = isl_range_text(a, (Range){ mod->name_first, mod->name_end });
    char names[96][K_SYMBOL_MAX];
    keel_slice_char whose[96];
    size_t nn = 0;
    for (size_t i = 0; i < a->symbols->len && nn < 96; i++) {
        const KSymbol *own = keel_buffer_KSymbol_ptr(a->symbols, i);
        if (!own->origin || !k_symtab_same_name(own->origin->name, self) || own->kind == K_SYM_MODULE) continue;
        char mine[K_TAGS_MAX_ITEMS + 1][K_SYMBOL_MAX];
        size_t nm = canonical_names(own, a, mine, K_TAGS_MAX_ITEMS + 1);
        bool told = false;
        for (size_t x = 0; x < nm && nn < 96; x++) {
            for (size_t y = 0; y < nn && !told; y++)
                if (!strcmp(names[y], mine[x]) && !k_symtab_same_name(whose[y], own->name)) {
                    Text t = { .ok = true };
                    isl_put_str(&t, mine[x]);
                    isl_diag3(c, K_DIAG_CANONICAL_NAME_COLLISION, own->name, isl_keep(c, &t), whose[y], own->name);
                    told = true;                        /* once per declaration */
                }
            memcpy(names[nn], mine[x], K_SYMBOL_MAX);
            whose[nn++] = own->name;
        }
    }
}

/* `constexpr T NAME = value;` (spec §4.2): the name is the token before the
   only `=` at the top (constexpr-name-missing), and the declaration is a
   scalar: no array declarator, no braces around the value (nonscalar-constexpr) */
void isl_check_constexpr(Ctx *c, size_t kw, size_t end) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t eq = SIZE_MAX, depth = 0;
    for (size_t j = kw + 1; j < end && !(depth == 0 && isl_punct(a, j, ";")); j++) {
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}")) depth--;
        else if (!depth && isl_punct(a, j, "=")) { eq = j; break; }
    }
    if (eq == SIZE_MAX) return;
    if (isl_punct(a, eq - 1, "]") || isl_punct(a, eq + 1, "{")) {
        isl_diag3(c, K_DIAG_NONSCALAR_CONSTEXPR, isl_tok(a, kw), none, none, none);
        return;
    }
    if (!isl_ident(a, eq - 1)) isl_diag3(c, K_DIAG_CONSTEXPR_NAME_MISSING, isl_tok(a, eq - 1), isl_tok(a, eq - 1), none, none);
}

/* ---- protocol declarations (spec §5.1, rules 5 and 7) ------------------ */

/* the protocol declared as `name` where `home` sees it, and the AST it is in */
static const KAstNode *protocol_node(const KAst *home, keel_slice_char name, const KAst **where) {
    *where = home;
    const KSymbol *s = home->symbols ? k_symtab_lookup(home->symbols, name) : NULL;
    if (s && s->origin && s->origin->ast) *where = s->origin->ast;
    for (size_t i = 0; i < (*where)->nodes.len; i++) {
        const KAstNode *n = isl_node_at(*where, i);
        if (n->kind == K_AST_PROTOCOL && k_symtab_same_name(isl_tok(*where, n->name_first), name)) return n;
    }
    return NULL;
}

static bool raw_protocol(const KAst *where, const KAstNode *n, KProtocolDecl *out) {
    size_t kw = n->first;
    while (kw < n->name_first && !isl_word(where, kw, "protocol")) kw++;
    KToken t = isl_tok(where, kw);
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, (keel_slice_char){ (size_t)(where->source.ptr + where->source.len - t.ptr), (char *)t.ptr }, NULL);
    KToken first = k_lexer_next(&lexer, &pp), next;
    KSymbol scratch[2];
    KSymbolTable none;
    k_symtab_init(&none, scratch, 2);
    return k_scan_decl_protocol(&lexer, first, &none, out, &next, &pp);
}

/* whether the composition under `name` reaches `target` again */
static bool reaches(const KAst *home, keel_slice_char name, keel_slice_char target, int depth) {
    if (depth > 8) return true;
    const KAst *where;
    const KAstNode *n = protocol_node(home, name, &where);
    KProtocolDecl pd;
    if (!n || !raw_protocol(where, n, &pd)) return false;
    for (size_t k = 0; k < pd.component_count; k++)
        if (k_symtab_same_name(pd.components[k], target) || reaches(where, pd.components[k], target, depth + 1)) return true;
    return false;
}

void isl_check_protocols(Ctx *c) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind != K_AST_PROTOCOL) continue;
        KProtocolDecl pd;
        if (!raw_protocol(a, n, &pd)) continue;
        bool sound = true;
        for (size_t k = 0; k < pd.component_count; k++) {
            const KAst *where;
            if (!protocol_node(a, pd.components[k], &where)) {
                isl_diag3(c, K_DIAG_UNKNOWN_PROTOCOL, pd.components[k], pd.components[k], none, none);
                sound = false;
            } else if (reaches(a, pd.components[k], pd.name, 0) || k_symtab_same_name(pd.components[k], pd.name)) {
                isl_diag3(c, K_DIAG_CIRCULAR_PROTOCOL, pd.components[k], pd.name, pd.components[k], none);
                sound = false;
            }
        }
        /* an associated type that no verb mentions binds nothing (rule 5) */
        for (size_t k = 0; k < pd.assoc_count; k++) {
            size_t uses = 0;
            for (size_t j = n->name_end; j < n->end; j++)
                if (isl_ident(a, j) && k_symtab_same_name(isl_tok(a, j), pd.assoc[k]) && !isl_word(a, j - 1, "type")) uses++;
            if (!uses) isl_diag3(c, K_DIAG_ASSOCIATED_TYPE_UNBOUND, pd.assoc[k], pd.assoc[k], pd.name, none);
        }
        if (!sound) continue;
        KProtocolDecl all;
        if (!isl_protocol_decl(c, a, pd.name, &all, 0)) continue;
        /* the union of the components: one signature per name and arity (rule 7) */
        for (size_t x = 0; x < all.verb_count; x++)
            for (size_t y = x + 1; y < all.verb_count; y++) {
                const KProtocolVerb *p = &all.verbs[x], *q = &all.verbs[y];
                if (!k_symtab_same_name(p->name, q->name) || p->arity != q->arity) continue;
                bool differ = p->receiver != q->receiver || p->result_role != q->result_role ||
                              !k_symtab_same_name(p->returns, q->returns);
                for (size_t r = 0; r < p->arity && r < K_PROTOCOL_MAX_PARAMS; r++) if (p->roles[r] != q->roles[r]) differ = true;
                if (differ) {
                    isl_diag3(c, K_DIAG_PROTOCOL_VERB_CONFLICT, isl_tok(a, n->name_first), p->name, pd.name, none);
                    x = all.verb_count;
                    break;
                }
            }
    }
}

/* ---- parameters `type` and over a protocol (spec §4.4, §4.14) --------- */

/* Spec §4.4, rules 10 to 18. A `type X` that names a `type` parameter of the
   module selects the instance, and then every one of them is named
   (partial-instance-selection); any other is erased: its name is no known
   type (type-param-shadows-type), and it only stands as `X *` in the
   signature and in `sizeof(X)` or `alignof(X)` in the body
   (type-param-outside-size). */
static void check_type_params(Ctx *c, const KAstNode *fn) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    const KAstNode *mod = isl_module_node(a);
    Param p[K_PARAMS_MAX];
    size_t np = isl_params_of(a, fn, p, K_PARAMS_MAX);
    if (np == SIZE_MAX) return;
    bool selects = false, covered[8] = {0};
    for (size_t k = 0; k < np; k++) {
        if (!p[k].is_type) continue;
        KToken x = isl_tok(a, p[k].end - 1);
        size_t b = 0;
        bool binder = false;
        for (size_t j = mod->type_first; j < mod->type_end; j++) {
            if (!isl_ident(a, j)) continue;
            if (k_symtab_same_name(isl_tok(a, j), x)) { binder = true; if (b < 8) covered[b] = true; }
            b++;
        }
        if (binder) { selects = true; continue; }
        const KSymbol *s = k_symbol_resolve(a->symbols, x);
        if (k_primitive_name(x) || (s && (s->kind == K_SYM_TYPE || s->kind == K_SYM_MODIFIER || s->kind == K_SYM_TAGS))) {
            isl_diag3(c, K_DIAG_TYPE_PARAM_SHADOWS_TYPE, x, x, none, none);
            continue;
        }
        size_t end = fn->body_end != SIZE_MAX ? fn->body_end : fn->end;
        for (size_t j = fn->first; j < end; j++) {
            if (j == p[k].end - 1 || !isl_ident(a, j) || !k_symtab_same_name(isl_tok(a, j), x)) continue;
            bool body = fn->body_first != SIZE_MAX && j > fn->body_first;
            bool ok = body ? (isl_punct(a, j - 1, "(") && isl_punct(a, j + 1, ")") &&
                              (isl_word(a, j - 2, "sizeof") || isl_word(a, j - 2, "alignof") || isl_word(a, j - 2, "_Alignof")))
                           : isl_punct(a, j + 1, "*");
            if (ok && body) {
                size_t open = j;
                for (size_t depth = 0; open > fn->body_first; open--) {
                    if (isl_punct(a, open, "]")) depth++;
                    else if (isl_punct(a, open, "[")) { if (!depth) { ok = false; break; } depth--; }
                    else if (isl_punct(a, open, ";") || isl_punct(a, open, "{")) break;
                }
            }
            if (!ok) { isl_diag3(c, K_DIAG_TYPE_PARAM_OUTSIDE_SIZE, isl_tok(a, j), x, x, x); break; }
        }
    }
    if (!selects) return;
    size_t b = 0;
    bool partial = mod->dim_first != mod->dim_end || mod->tags_first != mod->tags_end;
    for (size_t j = mod->type_first; j < mod->type_end; j++)
        if (isl_ident(a, j)) { if (b < 8 && !covered[b]) partial = true; b++; }
    if (partial) isl_diag3(c, K_DIAG_PARTIAL_INSTANCE_SELECTION, isl_tok(a, fn->name_first), isl_tok(a, fn->name_first), none, none);
}

/* Spec §4.14: in a function over a protocol, a verb called on the parameter
   is one the protocol requires (verb-not-in-protocol). */
static void check_protocol_body(Ctx *c, const KAstNode *fn) {
    KAst *a = c->ast;
    Param p[K_PARAMS_MAX];
    size_t np = isl_params_of(a, fn, p, K_PARAMS_MAX);
    if (np == SIZE_MAX || fn->body_first == SIZE_MAX) return;
    for (size_t k = 0; k < np; k++) {
        if (!p[k].is_protocol) continue;
        KProtocolDecl pd;
        if (!isl_protocol_decl(c, a, p[k].base, &pd, 0)) continue;
        KToken x = isl_tok(a, p[k].end - 1);
        for (size_t j = fn->body_first; j < fn->body_end; j++) {
            if (!isl_ident(a, j) || !isl_punct(a, j + 1, "(") || isl_punct(a, j - 1, ".") || isl_punct(a, j - 1, "->")) continue;
            size_t arg = j + 2;
            while (isl_punct(a, arg, "&") || isl_punct(a, arg, "*")) arg++;
            if (!k_symtab_same_name(isl_tok(a, arg), x) || !(isl_punct(a, arg + 1, ",") || isl_punct(a, arg + 1, ")"))) continue;
            size_t close = isl_close_paren(a, j + 1, fn->body_end);
            Range first;
            size_t arity = close == SIZE_MAX ? 0 : isl_call_args(a, j + 1, close, &first);
            bool known = false;
            for (size_t v = 0; v < pd.verb_count; v++)
                if (k_symtab_same_name(pd.verbs[v].name, isl_tok(a, j)) && pd.verbs[v].arity == arity) known = true;
            if (!known) isl_diag3(c, K_DIAG_VERB_NOT_IN_PROTOCOL, isl_tok(a, j), isl_tok(a, j), p[k].base, (keel_slice_char){0});
        }
    }
}

/* the '}' that closes the block opened at `open` */
static size_t block_end(const KAst *a, size_t open, size_t end) {
    for (size_t j = open, depth = 0; j < end; j++) {
        if (isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, "}") && --depth == 0) return j;
    }
    return end;
}

/* Spec §4.6, rule 9: an outside jump does not enter a scope over a `defer`
   registered in it — a `goto` to a label after the defer, from outside the
   block, or a `case`/`default` after a defer in the body of a `switch`
   (jump-over-defer). */
void isl_check_jumps(Ctx *c, const KAstNode *fn) {
    KAst *a = c->ast;
    if (fn->body_first == SIZE_MAX) return;
    for (size_t d = fn->body_first; d < fn->body_end; d++) {
        if (!isl_word(a, d, "defer")) continue;
        size_t open = isl_enclosing_brace(a, fn->body_first, d);
        if (open == SIZE_MAX) continue;
        size_t close = block_end(a, open, fn->body_end);
        const char *ctl = isl_control_before(a, fn->body_first, open);
        bool sw = ctl && !strcmp(ctl, "switch");
        for (size_t j = d + 1; j < close; j++) {
            if (isl_enclosing_brace(a, fn->body_first, j) != open) continue;      /* directly in the block */
            if (sw && (isl_word(a, j, "case") || (isl_word(a, j, "default") && isl_punct(a, j + 1, ":")))) {
                isl_diag3(c, K_DIAG_JUMP_OVER_DEFER, isl_tok(a, j), isl_tok(a, j), (keel_slice_char){0}, (keel_slice_char){0});
                continue;
            }
            if (!isl_ident(a, j) || !isl_punct(a, j + 1, ":") || isl_punct(a, j + 2, ":")) continue;
            if (!(isl_punct(a, j - 1, ";") || isl_punct(a, j - 1, "{") || isl_punct(a, j - 1, "}"))) continue;
            for (size_t g = fn->body_first; g + 1 < fn->body_end; g++)
                if (isl_word(a, g, "goto") && k_symtab_same_name(isl_tok(a, g + 1), isl_tok(a, j)) && (g < open || g > close))
                    isl_diag3(c, K_DIAG_JUMP_OVER_DEFER, isl_tok(a, g), isl_tok(a, j), (keel_slice_char){0}, (keel_slice_char){0});
        }
    }
}

/* ---- layout (backend §4.3.1) ------------------------------------------- */

/* whether a modifier of `home` holds a field of binder `b` by value */
static bool binder_by_value(const KAst *home, keel_slice_char b) {
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *m = isl_node_at(home, i);
        if (m->kind != K_AST_MODIFIER || m->body_first == SIZE_MAX) continue;
        for (size_t j = m->body_first; j + 1 < m->body_end; j++)
            if (k_symtab_same_name(isl_tok(home, j), b) && isl_ident(home, j + 1) &&
                (j == m->body_first || isl_punct(home, j - 1, ";") || isl_punct(home, j - 1, "{")))
                return true;
    }
    return false;
}

/* whether the type written as `text` holds `target` by value, through the
   fields by value of the modifiers it applies */
static bool holds(const KAst *a, keel_slice_char text, keel_slice_char target, Text *chain, int depth) {
    if (depth > 8) return false;
    KLexer lexer;
    TKPpKind pp;
    KSpecifier spec;
    KToken next = {0};
    KToken first = isl_enter_text(text, &lexer, &pp);
    if (k_token_spelled(first, "struct")) first = k_lexer_next(&lexer, &pp);
    if (!first.len || !k_token_is_ident(first)) return false;
    if (!k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) || spec.kind == K_SPEC_NONE)
        return k_symtab_same_name(first, target);
    if (spec.kind == K_SPEC_NAMED_TYPE) return k_symtab_same_name(spec.type_name, target);
    const KSymbol *m = k_symbol_resolve(a->symbols, spec.modifier_name);
    const KAst *home = m && m->origin ? m->origin->ast : NULL;
    if (!home) return false;
    keel_slice_char binders[8];
    size_t nb = isl_binders_of(home, binders);
    for (size_t k = 0; k < nb && k < spec.arg_count; k++)
        if (binder_by_value(home, binders[k]) && holds(a, spec.args[k], target, chain, depth + 1)) {
            isl_put_str(chain, " \xe2\x86\x92 ");
            isl_put(chain, spec.text.ptr, spec.text.len);
            return true;
        }
    return false;
}

/* a struct of the file that holds itself by value through an instance of a
   modifier: a type of infinite size (layout-cycle) */
void isl_check_layout(Ctx *c) {
    KAst *a = c->ast;
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind != K_AST_TYPE || n->body_first == SIZE_MAX || n->name_first == n->name_end) continue;
        KToken self = isl_tok(a, n->name_first);
        for (size_t f = n->body_first; f < n->body_end; ) {
            size_t e = f, depth = 0;
            while (e < n->body_end && !(depth == 0 && isl_punct(a, e, ";"))) {
                if (isl_punct(a, e, "(") || isl_punct(a, e, "[") || isl_punct(a, e, "{")) depth++;
                else if (isl_punct(a, e, ")") || isl_punct(a, e, "]") || isl_punct(a, e, "}")) depth--;
                e++;
            }
            size_t start = f;
            if (isl_punct(a, start, "{")) start++;
            KLexer lexer;
            TKPpKind pp;
            KSpecifier spec;
            KToken next = {0};
            KToken first = start < e ? isl_enter(a, start, &lexer, &pp) : (KToken){0};
            if (first.len && k_token_is_ident(first) &&
                k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind == K_SPEC_MODIFIER &&
                !k_token_is_punct(next, "*")) {
                Text chain = { .ok = true };
                if (holds(a, spec.text, self, &chain, 0)) {
                    Text all = { .ok = true };
                    isl_put(&all, self.ptr, self.len);
                    isl_put(&all, chain.s, chain.n);
                    isl_put_str(&all, " \xe2\x86\x92 ");
                    isl_put(&all, self.ptr, self.len);
                    isl_diag3(c, K_DIAG_LAYOUT_CYCLE, first, isl_keep(c, &all), (keel_slice_char){0}, (keel_slice_char){0});
                }
            }
            f = e + 1;
        }
    }
}
