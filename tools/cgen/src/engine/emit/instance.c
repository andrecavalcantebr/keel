/* engine/emit/instance.c — the headers of an instance of a generic module
 * (codegen design §7; backend §4.3.2, §4.4.1). The instance is the declarations
 * of the generic module that name a binder or a modifier (k_em_generic_out),
 * copied from the generic source with the binders replaced by the arguments and
 * the module's names by the instance's:
 *
 *   .type.h  the modifier, as `typedef struct <symbol> { ... } <symbol>;`;
 *   .h       the prototype of each `pub` function, then the bodies of the
 *            `pub inline` ones (`static inline`); the body of a `pub` without
 *            `inline` is the business of the module that places the instance.
 *
 * A type or a constant that names a parameter goes to the .type.h of the instance,
 * with the header's name as prefix (backend §4.3, rule 1). A `priv inline`
 * function is `static inline` in the .h, prototype and body, under the instance's
 * name (backend §4.3, rule 6).
 *
 * Refused (K_EMIT_UNSUPPORTED): a prototype or a constant that spans lines; a
 * generic module with dimension or tag binders. A modifier may span lines (its
 * line breaks stay, one `#line` for the block), and a comment inside a
 * declaration leaves the text with its blanks as written (backend §6, rule 3).
 *
 * A `priv` function that is not inline is the instance's, and not the header's:
 * its body goes to the .c of the module that places the instance, as `static`
 * (backend §4.4). */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define EM_DASH "\xE2\x80\x94"  /* U+2014: the dash of every header comment */

static bool same_text(KToken a, KToken b) { return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0; }

static bool same_slice(KToken t, keel_slice_char s) { return t.len == s.len && memcmp(t.ptr, s.ptr, s.len) == 0; }

static void raw(Emit *e, const char *s) { k_emit_raw(&e->w, (keel_slice_char){ strlen(s), (char *)s }); }
static void raw_slice(Emit *e, keel_slice_char s) { k_emit_raw(&e->w, s); }

static void put_slice(Emit *e, keel_slice_char s) { k_em_put_bytes(e, s.ptr, s.len); }

/* the number of tags binders of the module line */
static size_t tags_count(const KAst *ast) {
    const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    size_t k = 0;
    for (size_t j = m->tags_first; j < m->tags_end; j++) {
        KToken x = k_em_tok(ast, j);
        if (k_token_is_ident(x) && !k_em_text_is(x, "tags")) k++;
    }
    return k;
}

/* the argument bound to the binder spelled like `t` (a dimension: its value;
 * a type: its symbol), or false */
bool k_em_binder_arg(const Emit *e, KToken t, keel_slice_char *arg);

keel_slice_char k_em_unqualified(keel_slice_char s) {
    for (;;) {
        if (s.len > 6 && memcmp(s.ptr, "const ", 6) == 0) { s.ptr += 6; s.len -= 6; continue; }
        if (s.len > 9 && memcmp(s.ptr, "volatile ", 9) == 0) { s.ptr += 9; s.len -= 9; continue; }
        return s;
    }
}
#define binder_arg k_em_binder_arg
bool k_em_binder_arg(const Emit *e, KToken t, keel_slice_char *arg) {
    const KAst *ast = e->ast;
    const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    /* the arguments run in the order of the module line: tags, then types */
    size_t first[3] = { m->dim_first, m->tags_first, m->type_first }, end[3] = { m->dim_end, m->tags_end, m->type_end };
    size_t k = 0;
    for (int r = 0; r < 3; r++) {
        if (r == 1) k = 0;
        for (size_t j = first[r]; j < end[r]; j++) {
            KToken x = k_em_tok(ast, j);
            if (!k_token_is_ident(x) || k_em_text_is(x, "dim") || k_em_text_is(x, "type") || k_em_text_is(x, "tags")) continue;
            if (same_text(x, t)) {
                if (r == 0) {
                    if (k >= e->inst->dimc) return false;
                    *arg = e->inst->dim[k];
                    return true;
                }
                if (k >= e->inst->argc) return false;
                *arg = e->inst->arg[k];
                /* a type argument as C writes it (`const char`), when the closure has it */
                if (e->inst->arg_spell[k].len) *arg = e->inst->arg_spell[k];
                return true;
            }
            k++;
        }
        if (r == 1) k = tags_count(ast);
    }
    return false;
}

/* the number of binders in [first, end) of the module line */
static size_t binders(const KAst *ast, size_t first, size_t end) {
    size_t k = 0;
    for (size_t j = first; j < end; j++) {
        KToken x = k_em_tok(ast, j);
        if (k_token_is_ident(x) && !k_em_text_is(x, "dim") && !k_em_text_is(x, "type")) k++;
    }
    return k;
}

/* The rank suffix of a vector accessor (backend §2.1.1, rule 6): the function's
 * second parameter is `size_t idx[static N]`, with N a `dim` binder, and the
 * suffix is `_<value of N>`. False for any other function. */
#define rank_of k_em_rank_suffix
bool k_em_rank_suffix(const Emit *e, const KAstNode *fn, keel_slice_char *value) {
    const KAst *ast = e->ast;
    if (fn->kind != K_AST_FUNCTION || fn->name_first + 1 >= fn->end || !k_em_text_is(k_em_tok(ast, fn->name_first + 1), "(")) return false;
    size_t depth = 0, param = 0;
    for (size_t j = fn->name_first + 2; j + 3 < fn->end; j++) {
        KToken t = k_em_tok(ast, j);
        if (k_em_text_is(t, "(")) depth++;
        else if (k_em_text_is(t, ")")) { if (depth == 0) break; depth--; }
        else if (depth == 0 && k_em_text_is(t, ",")) param++;
        else if (param == 1 && k_em_text_is(t, "[") && k_em_text_is(k_em_tok(ast, j + 1), "static") &&
                 k_em_text_is(k_em_tok(ast, j + 3), "]")) {
            const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
            size_t k = 0;
            for (size_t b = m->dim_first; b < m->dim_end; b++) {
                KToken x = k_em_tok(ast, b);
                if (!k_token_is_ident(x) || k_em_text_is(x, "dim")) continue;
                if (same_text(x, k_em_tok(ast, j + 2))) {
                    if (k >= e->inst->dimc) return false;
                    *value = e->inst->dim[k];
                    return true;
                }
                k++;
            }
        }
    }
    return false;
}

/* Whether the function takes the modifier itself as the type of a parameter
 * (`const slot *s`; `slice slot s` does not: its type is the slice's), the same
 * test as the parser's receiver. In an instance that is a unit of its own, such
 * a verb carries the modifier's symbol, the others the unit's (backend §5.10, rule 1). */
bool k_em_verb_on_modifier(const Emit *e, const KAstNode *fn) {
    const KAst *ast = e->ast;
    if (!e->inst->unit_len) return true;
    if (fn->kind != K_AST_FUNCTION || fn->name_first + 1 >= fn->end || !k_em_text_is(k_em_tok(ast, fn->name_first + 1), "(")) return true;
    size_t depth = 0;
    bool head = true;
    for (size_t j = fn->name_first + 2; j < fn->end; j++) {
        KToken t = k_em_tok(ast, j);
        if (k_em_text_is(t, "(")) depth++;
        else if (k_em_text_is(t, ")")) { if (depth == 0) break; depth--; }
        else if (depth == 0 && k_em_text_is(t, ",")) head = true;
        else if (depth == 0 && head && !k_em_text_is(t, "const") && !k_em_text_is(t, "volatile")) {
            if (same_slice(t, e->inst->modifier)) return true;
            head = false;
        }
    }
    return false;
}

/* the last component of the module name `m` ("keel.routine" -> "routine") */
static keel_slice_char last_component(keel_slice_char m) {
    size_t last = m.len;
    while (last > 0 && m.ptr[last - 1] != '.') last--;
    return (keel_slice_char){ m.len - last, m.ptr + last };
}

/* Whether `n` is the typedef that names the instance's unit: a typedef spelled
 * like the module's last component, as `routine` in keel.routine (backend §5.10,
 * rule 1). Any other typedef of a generic module is a declaration of the
 * instance, named by its header (backend §4.3, rule 1). */
static bool unit_typedef(const KAst *ast, const KAstNode *n, keel_slice_char m) {
    if (n->kind != K_AST_TYPE || n->name_first >= n->name_end) return false;
    KToken t = k_em_tok(ast, n->name_first);
    keel_slice_char last = last_component(m);
    return t.len == last.len && memcmp(t.ptr, last.ptr, t.len) == 0;
}

bool k_em_unit_typedef(const Emit *e, const KAstNode *d) {
    return e->inst && unit_typedef(e->ast, d, e->inst->module);
}

size_t k_em_unit_symbol(const KAst *user, const KClosure *c, char *buf, size_t cap) {
    if (!c->module || !c->module->ast || !c->modifier.len || !k_em_module_is_generic(c->module->ast)) return 0;
    const KAst *g = c->module->ast;
    keel_slice_char m = c->module->name;
    size_t last = m.len;
    while (last > 0 && m.ptr[last - 1] != '.') last--;
    if (m.len - last == c->modifier.len && memcmp(m.ptr + last, c->modifier.ptr, c->modifier.len) == 0) return 0;
    bool typedefs = false;
    for (size_t i = 0; i < g->nodes.len && !typedefs; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&g->nodes, i);
        typedefs = unit_typedef(g, n, m) && k_em_generic_out(g, n);
    }
    if (!typedefs) return 0;
    size_t n = k_mangle_module(m, (keel_slice_char){ cap, buf });
    for (size_t k = 0; k < c->argc && n <= cap; k++) {
        if (n + 1 + c->arg_len[k] > cap) return 0;
        buf[n++] = '_';
        memcpy(buf + n, user->closure_text.ptr + c->arg_text[k], c->arg_len[k]);
        n += c->arg_len[k];
    }
    return n <= cap ? n : 0;
}

/* token `i` of the generic source, as the instance spells it; in the fields of
 * the modifier (`fields`) a name is a member, and only a type is renamed */
static void put_token(Emit *e, size_t i, bool fields) {
    const KAst *ast = e->ast;
    KToken t = k_em_tok(ast, i);
    bool member = i > 0 && (k_em_text_is(k_em_tok(ast, i - 1), ".") || k_em_text_is(k_em_tok(ast, i - 1), "->"));
    keel_slice_char arg;
    if (!member && k_token_is_ident(t)) {
        if (binder_arg(e, t, &arg)) { put_slice(e, arg); return; }
        if (same_slice(t, e->inst->modifier)) { put_slice(e, e->inst->symbol); return; }
        const KAstNode *d = k_em_declared_at(e, i);
        if (d && fields && d->kind != K_AST_TYPE && d->kind != K_AST_TAGS) d = NULL;
        if (d) {
            if (k_em_generic_out(ast, d) && k_em_unit_typedef(e, d)) {
                put_slice(e, k_emit_unit(e->inst));   /* the unit's own typedef (backend §5.10) */
            } else if (k_em_generic_out(ast, d)) {
                put_slice(e, d->kind == K_AST_FUNCTION && !k_em_verb_on_modifier(e, d) ? k_emit_unit(e->inst) : e->inst->symbol);
                k_em_put_bytes(e, "_", 1);
                k_em_put_bytes(e, t.ptr, t.len);
                keel_slice_char rank;
                if (rank_of(e, d, &rank)) { k_em_put_bytes(e, "_", 1); put_slice(e, rank); }
            } else {
                k_em_put_mangled(e, d->name_first, d->kind == K_AST_TYPE || d->kind == K_AST_TAGS);
            }
            return;
        }
    }
    k_em_put_bytes(e, t.ptr, t.len);
}

/* A gap between two tokens: blanks and breaks, as written. A comment leaves its
 * text and nothing else: the blanks around it stay (backend §6, rule 3), and so
 * does the break that ends a `//` comment. A block comment over several lines
 * would drop breaks and shift the .k lines: refused. */
static void put_gap(Emit *e, size_t g0, size_t g1) {
    const char *s = e->ast->source.ptr;
    size_t p = g0;
    while (p < g1 && e->ok) {
        if (s[p] == '/' && p + 1 < g1 && s[p + 1] == '/') {
            while (p < g1 && s[p] != '\n') p++;
        } else if (s[p] == '/' && p + 1 < g1 && s[p + 1] == '*') {
            size_t ce = p + 2;
            while (ce + 1 < g1 && !(s[ce] == '*' && s[ce + 1] == '/')) ce++;
            ce = ce + 2 <= g1 ? ce + 2 : g1;
            if (memchr(s + p, '\n', ce - p)) { e->ok = false; return; }
            p = ce;
        } else {
            k_em_put_bytes(e, s + p, 1);
            p++;
        }
    }
}

/* The tokens of `n` in the source bytes [a, b), with the gaps as written. The
 * visibility word is dropped with the blanks after it (vis_static false), or
 * becomes `static` (true). `ed` are the renames of another module's names
 * (k_em_edits_uses), by position: a replaced name is written whole. */
static void put_span(Emit *e, const KAstNode *n, size_t a, size_t b, bool vis_static, const Edit *ed, size_t ned) {
    bool fields = n->kind == K_AST_MODIFIER;
    const KAst *ast = e->ast;
    size_t pos = a;
    for (size_t i = n->first; i < n->end && e->ok; i++) {
        size_t off = k_em_tok_off(ast, i), end = k_em_tok_end(ast, i);
        if (end <= a) continue;
        if (off >= b) break;
        if (i == n->first && k_em_is_visibility(k_em_tok(ast, i))) {
            if (vis_static) { put_gap(e, pos, off); k_em_put_bytes(e, "static", 6); }
            else if (i + 1 < n->end) { pos = k_em_tok_off(ast, i + 1); continue; }
            pos = end;
            continue;
        }
        /* a name the generic declares for its instances (the typedef) is the unit's, whatever
           island the parser put there */
        const KAstNode *own = k_em_declared_at(e, i);
        size_t x = own && k_em_generic_out(ast, own) ? ned : 0;
        while (x < ned && !(ed[x].at == off && ed[x].kind == EDIT_TEXT && ed[x].skip > 0)) x++;
        if (x < ned) {
            put_gap(e, pos, off);
            k_em_put_bytes(e, ed[x].text, ed[x].text_len);
            pos = off + ed[x].skip;
            while (i + 1 < n->end && k_em_tok_off(ast, i + 1) < pos) i++;
            continue;
        }
        put_gap(e, pos, off);
        put_token(e, i, fields);
        pos = end;
    }
    if (e->ok && pos < b) put_gap(e, pos, b);
}

static bool one_line(const Emit *e, size_t a, size_t b) {
    return memchr(e->ast->source.ptr + a, '\n', b - a) == NULL;
}

/* writes e->buf as the .k line `ln`, and empties it */
static void flush(Emit *e, size_t ln) {
    if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
    e->n = 0;
}

/* `pub modifier stack byref { ... }` -> `typedef struct S { ... } S;` */
static void emit_modifier(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t open = n->first, close = n->end;
    while (open < n->end && !k_em_text_is(k_em_tok(ast, open), "{")) open++;
    while (close > open && !k_em_text_is(k_em_tok(ast, close - 1), "}")) close--;
    if (open >= n->end || close <= open + 1) { e->ok = false; return; }
    /* between the name and the `{` only `byref` */
    for (size_t j = n->name_first + 1; j < open; j++)
        if (!k_em_text_is(k_em_tok(ast, j), "byref")) { e->ok = false; return; }
    size_t a = k_em_tok_off(ast, open), b = k_em_tok_end(ast, close - 1);
    Edit ed[MAX_EDITS];
    size_t ned = 0;
    k_em_edits_uses(e, n, a, b, ed, &ned);
    k_em_sort_edits(ed, 0, ned);
    e->n = 0;
    k_em_put_bytes(e, "typedef struct ", 15);
    put_slice(e, e->inst->symbol);
    k_em_put_bytes(e, " ", 1);
    /* a value field of a binder bound to `void` is omitted, with the blanks
     * after it (spec §4.3, rule 11); `T *field` stays */
    size_t from = a;
    for (size_t j = open + 1; j + 2 < close && e->ok; j++) {
        keel_slice_char arg;
        KToken t = k_em_tok(ast, j);
        if (!k_token_is_ident(t) || !binder_arg(e, t, &arg) || arg.len != 4 || memcmp(arg.ptr, "void", 4) != 0) continue;
        if (!k_token_is_ident(k_em_tok(ast, j + 1)) || !k_em_text_is(k_em_tok(ast, j + 2), ";")) continue;
        put_span(e, n, from, k_em_tok_off(ast, j), false, ed, ned);
        from = k_em_tok_off(ast, j + 3);
    }
    put_span(e, n, from, b, false, ed, ned);
    k_em_put_bytes(e, " ", 1);
    put_slice(e, e->inst->symbol);
    k_em_put_bytes(e, ";\n", 2);
    flush(e, k_em_line_at(e, k_em_first_off(e, n)));
}

/* `pub typedef corot (*routine)(C *ctx);` -> the same line: no visibility word, the
 * binders replaced, the module's names by the instance's */
static void emit_typedef(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t a = k_em_first_off(e, n), b = k_em_last_end(e, n);
    if (!one_line(e, a, b)) { e->ok = false; return; }
    Edit ed[MAX_EDITS];
    size_t ned = 0;
    k_em_edits_uses(e, n, a, b, ed, &ned);
    k_em_sort_edits(ed, 0, ned);
    e->n = 0;
    /* `typedef struct { … } Name;`: the anonymous struct takes the tag of its first
     * declarator (backend §2.1, rule 6), the instance's name for it */
    size_t s = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
    bool anonymous = s + 2 < n->end && k_em_text_is(k_em_tok(ast, s), "typedef") &&
                     k_em_text_is(k_em_tok(ast, s + 1), "struct") && k_em_text_is(k_em_tok(ast, s + 2), "{") &&
                     n->name_first < n->name_end;
    if (anonymous) {
        size_t cut = k_em_tok_end(ast, s + 1);
        put_span(e, n, a, cut, false, ed, ned);
        k_em_put_bytes(e, " ", 1);
        put_token(e, n->name_first, false);
        k_em_put_bytes(e, " ", 1);
        put_span(e, n, k_em_tok_off(ast, s + 2), b, false, ed, ned);
    } else {
        put_span(e, n, a, b, false, ed, ned);
    }
    k_em_put_bytes(e, "\n", 1);
    flush(e, k_em_line_at(e, a));
}

/* `constexpr T N = INIT;` of the generic, as the instance's constant (backend §4.3,
 * rule 1): under C23 the declaration with the instance's name; under C11 the form
 * of §9.2, a macro of that name and the checking object, both on its line. */
static void emit_constexpr(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t a = k_em_first_off(e, n), b = k_em_last_end(e, n);
    if (!one_line(e, a, b)) { e->ok = false; return; }
    e->n = 0;
    if (e->opt.c23) {
        put_span(e, n, a, b, false, NULL, 0);
        k_em_put_bytes(e, "\n", 1);
        flush(e, k_em_line_at(e, a));
        return;
    }
    size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0) + 1;   /* past `constexpr` */
    size_t ts = k_em_tok_off(ast, k), te = k_em_tok_off(ast, n->name_first);
    while (te > ts && (ast->source.ptr[te - 1] == ' ' || ast->source.ptr[te - 1] == '\t')) te--;
    bool paren = n->end - 2 > n->name_first + 2;   /* the INIT is more than one token */
    size_t ia = k_em_tok_off(ast, n->name_first + 2), ib = k_em_tok_end(ast, n->end - 2);
    size_t ln = k_em_line_at(e, a);
    k_em_put_bytes(e, "#define ", 8);
    put_token(e, n->name_first, false);
    k_em_put_bytes(e, " ((", 3);
    k_em_put_bytes(e, ast->source.ptr + ts, te - ts);
    k_em_put_bytes(e, ")", 1);
    if (paren) k_em_put_bytes(e, "(", 1);
    put_span(e, n, ia, ib, false, NULL, 0);
    if (paren) k_em_put_bytes(e, ")", 1);
    k_em_put_bytes(e, ")\n", 2);
    flush(e, ln);
    k_em_put_bytes(e, "static const ", 13);
    k_em_put_bytes(e, ast->source.ptr + ts, te - ts);
    k_em_put_bytes(e, " ", 1);
    put_token(e, n->name_first, false);
    k_em_put_bytes(e, "__chk = ", 8);
    if (paren) k_em_put_bytes(e, "(", 1);
    put_span(e, n, ia, ib, false, NULL, 0);
    if (paren) k_em_put_bytes(e, ")", 1);
    k_em_put_bytes(e, "; _Static_assert(sizeof(", 24);
    put_token(e, n->name_first, false);
    k_em_put_bytes(e, "__chk) > 0, \"constexpr\");\n", 26);
    flush(e, ln);
}

/* the declarations of the generic that name a parameter, and go to the .type.h of
 * the instance, in the order of the source (backend §4.3, rule 1): a constant, a
 * typedef or the modifier */
static bool type_h_node(const KAst *ast, const KAstNode *n) {
    return k_em_generic_out(ast, n) && (n->kind == K_AST_TYPE || n->kind == K_AST_CONSTEXPR);
}

/* whether an earlier node of the tree shares the span of `n` (the declarators of
 * one declaration, `typedef struct {…} P, *PP;`): the first one writes it all */
static bool shares_earlier_span(const KAst *ast, const KAstNode *n) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *d = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (d == n) return false;
        if (d->first == n->first) return true;
    }
    return false;
}

static bool is_function_with_body(const Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    return n->kind == K_AST_FUNCTION && n->body_first != (size_t)-1 && n->body_first >= 2 &&
           k_em_text_is(k_em_tok(ast, n->body_first - 1), "{") && k_em_text_is(k_em_tok(ast, n->body_first - 2), ")");
}

/* the prototype of a `pub` function of the instance */
static void emit_proto(Emit *e, const KAstNode *n) {
    size_t a = k_em_first_off(e, n), b = k_em_tok_end(e->ast, n->body_first - 2);
    if (!one_line(e, a, b)) { e->ok = false; return; }
    e->n = 0;
    put_span(e, n, a, b, n->is_inline, NULL, 0);
    k_em_put_bytes(e, ";\n", 2);
    flush(e, k_em_line_at(e, a));
}

/* the body of a `pub` function: its source lines, one at a time; a `pub inline`
 * one is `static inline` (vis_static), the other loses the visibility word */
static void emit_body(Emit *e, const KAstNode *n, bool vis_static) {
    size_t a = k_em_line_start(e, k_em_first_off(e, n)), end = k_em_last_end(e, n);
    size_t ln = k_em_line_at(e, a);
    while (a < end && e->ok) {
        size_t next = k_em_line_end(e, a);
        e->n = 0;
        put_span(e, n, a, next, vis_static, NULL, 0);
        if (e->ok && (e->n == 0 || e->buf[e->n - 1] != '\n')) k_em_put_bytes(e, "\n", 1);
        flush(e, ln);
        a = next;
        ln++;
    }
}

/* whether the declaration is one the instance can carry */
static bool carried(const Emit *e, const KAstNode *n) {
    if (n->kind == K_AST_MODIFIER || n->kind == K_AST_TYPE || n->kind == K_AST_CONSTEXPR) return true;
    return is_function_with_body(e, n);
}

static bool has_modifier(const KAst *ast) {
    for (size_t i = 0; i < ast->nodes.len; i++)
        if (keel_buffer_KAstNode_ptr(&ast->nodes, i)->kind == K_AST_MODIFIER) return true;
    return false;
}

static bool instance_supported(const Emit *e) {
    const KAst *ast = e->ast;
    const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    if (e->inst->argc > K_EMIT_INSTANCE_ARGS || e->inst->dimc > K_EMIT_INSTANCE_ARGS) return false;
    if (binders(ast, m->dim_first, m->dim_end) != e->inst->dimc ||
        tags_count(ast) + binders(ast, m->type_first, m->type_end) != e->inst->argc)
        return false;
    bool modifier = false;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->first >= n->end || !k_em_generic_out(ast, n) || k_em_other_modifier(e, n)) continue;
        if (!carried(e, n)) return false;
        if (n->kind == K_AST_MODIFIER) {
            if (!same_slice(k_em_tok(ast, n->name_first), e->inst->modifier)) return false;
            modifier = true;
        }
    }
    /* a generic without a modifier (`keel.array`, backend §5.2, rule 8): the
     * instance is the family of verbs of its arguments */
    return modifier || (!has_modifier(ast) && !e->inst->modifier.len);
}

/* a `pub` function: its prototype is in the .h (a `priv` one is not) */
static bool has_function(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (k_em_generic_out(e->ast, n) && n->kind == K_AST_FUNCTION && n->is_public && !k_em_other_modifier(e, n)) return true;
    }
    return false;
}

static bool has_inline(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (k_em_generic_out(e->ast, n) && n->kind == K_AST_FUNCTION && n->is_inline && !k_em_other_modifier(e, n)) return true;
    }
    return false;
}

/* How the modifier's fields hold type binder k: 0 not at all, 1 only by
 * pointer, 2 by value */
int k_em_binder_use(const Emit *e, size_t k);
#define binder_use_in_fields k_em_binder_use
int k_em_binder_use(const Emit *e, size_t k) {
    const KAst *ast = e->ast;
    const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    KToken name = {0};
    size_t q = 0;
    for (size_t j = m->type_first; j < m->type_end; j++) {
        KToken x = k_em_tok(ast, j);
        if (!k_token_is_ident(x) || k_em_text_is(x, "type")) continue;
        if (q++ == k) { name = x; break; }
    }
    if (!name.len) return 0;
    int use = 0;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->kind != K_AST_MODIFIER || k_em_other_modifier(e, n)) continue;
        for (size_t j = n->name_first + 1; j < n->end; j++)
            if (same_text(k_em_tok(ast, j), name)) {
                if (j + 1 < n->end && k_em_text_is(k_em_tok(ast, j + 1), "*")) { if (use < 1) use = 1; }
                else use = 2;
            }
    }
    return use;
}

static void emit_file(Emit *e, KEmitFile which) {
    bool type_h = which == K_EMIT_TYPE_H;
    char path_buf[NAME_CAP], guard_buf[NAME_CAP], type_buf[NAME_CAP];
    size_t pn = k_mangle_instance_path(e->inst->module, k_emit_unit(e->inst), type_h ? ".type.h" : ".h", (keel_slice_char){ NAME_CAP, path_buf });
    size_t tn = k_mangle_instance_path(e->inst->module, k_emit_unit(e->inst), ".type.h", (keel_slice_char){ NAME_CAP, type_buf });
    if (pn > NAME_CAP || tn > NAME_CAP) { e->ok = false; return; }
    keel_slice_char path = { pn, path_buf }, type_path = { tn, type_buf };
    size_t gn = k_mangle_guard(path, (keel_slice_char){ NAME_CAP, guard_buf });
    if (gn > NAME_CAP) { e->ok = false; return; }
    keel_slice_char guard = { gn, guard_buf };

    raw(e, "/* ");
    raw_slice(e, path);
    raw(e, " " EM_DASH " generated from ");
    raw_slice(e, e->src_path);
    raw(e, e->opt.c23 ? " by cgen, C23 profile. */\n" : " by cgen, C11 profile. */\n");
    raw(e, "#ifndef "); raw_slice(e, guard);
    raw(e, "\n#define "); raw_slice(e, guard);
    raw(e, "\n\n");
    if (type_h) {
        raw(e, "#include \"keel.type.h\"\n");
        /* an argument that is an instance: its .type.h when a field holds it by
         * value, a declaration ahead when only by pointer (backend §4.3.2, rule 2) */
        if (e->inst->unit_len) k_em_instance_type_includes(e);
        bool ahead = false;
        for (int pass = 0; pass < 2; pass++)
            for (size_t k = 0; k < e->inst->argc && e->ok && !e->inst->unit_len; k++) {
                if (!e->inst->arg_module[k].len) continue;
                int use = binder_use_in_fields(e, k);
                if (use == 2 && pass == 0) {
                    char path[NAME_CAP];
                    size_t pl = k_mangle_instance_path(e->inst->arg_module[k], k_emit_arg_file(e->inst, k), ".type.h", (keel_slice_char){ sizeof path, path });
                    if (pl > sizeof path) { e->ok = false; return; }
                    raw(e, "#include \""); raw_slice(e, (keel_slice_char){ pl, path }); raw(e, "\"\n");
                } else if (use == 1 && pass == 1) {
                    /* after the includes and a blank (E6, R1); the declarations that follow take no blank (R6) */
                    if (!ahead) raw(e, "\n");
                    ahead = true;
                    raw(e, "typedef struct "); raw_slice(e, e->inst->arg[k]); raw(e, " "); raw_slice(e, e->inst->arg[k]); raw(e, ";\n");
                }
            }
        /* without a modifier the instance defines no type: the header only
         * includes keel.type.h (backend §5.2, rule 8) */
        if (has_modifier(e->ast) && !e->inst->proto_fn.len && !ahead) raw(e, "\n");
        for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (n->kind == K_AST_MODIFIER && !e->inst->proto_fn.len && !k_em_other_modifier(e, n)) emit_modifier(e, n);
            else if (type_h_node(e->ast, n) && !shares_earlier_span(e->ast, n)) {
                if (n->kind == K_AST_CONSTEXPR) emit_constexpr(e, n);
                else emit_typedef(e, n);
            }
        }
    } else {
        raw(e, "#include \""); raw_slice(e, type_path); raw(e, "\"\n");
        if (has_function(e)) raw(e, "\n");
        for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (k_em_generic_out(e->ast, n) && n->kind == K_AST_FUNCTION && n->is_public) emit_proto(e, n);
        }
        if (has_inline(e)) raw(e, "\n");
        for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (k_em_generic_out(e->ast, n) && n->kind == K_AST_FUNCTION && n->is_inline) emit_body(e, n, true);
        }
    }
    raw(e, "\n#endif /* "); raw_slice(e, guard); raw(e, " */\n");
}

static bool is_base(keel_slice_char m) {
    return m.len >= 4 && memcmp(m.ptr, "keel", 4) == 0 && (m.len == 4 || m.ptr[4] == '.');
}

bool k_emit_instance_of(const KAst *ast, const KClosure *c, KEmitInstance *inst) {
    if (c->kind != K_CLOSURE_INSTANCE || !c->module || !c->module->ast || c->argc > K_EMIT_INSTANCE_ARGS) return false;
    *inst = (KEmitInstance){ .symbol = { c->symbol_len, (char *)c->symbol }, .module = c->module->name,
                             .modifier = c->modifier, .argc = c->argc,
                             .unavailable = c->unavailable_count ? keel_buffer_KUnavailable_ptr(&ast->unavailable, c->unavailable_first) : NULL,
                             .unavailable_count = c->unavailable_count };
    for (size_t k = 0; k < c->argc; k++) {
        inst->arg[k] = (keel_slice_char){ c->arg_len[k], ast->closure_text.ptr + c->arg_text[k] };
        inst->arg_spell[k] = c->arg_spell_len[k] ? (keel_slice_char){ c->arg_spell_len[k], ast->closure_text.ptr + c->arg_spell[k] }
                                                 : inst->arg[k];
        /* an argument that is itself an instance of the closure: its module names its header */
        for (size_t q = 0; q < ast->closure.len; q++) {
            const KClosure *o = keel_buffer_KClosure_ptr(&ast->closure, q);
            if (o->kind == K_CLOSURE_INSTANCE && o->module && o->symbol_len == inst->arg[k].len &&
                memcmp(o->symbol, inst->arg[k].ptr, o->symbol_len) == 0)
            { inst->arg_module[k] = o->module->name;
              inst->arg_unit_len[k] = k_em_unit_symbol(ast, o, inst->arg_unit_buf[k], sizeof inst->arg_unit_buf[k]); }
        }
    }
    inst->user = ast;
    inst->unit_len = k_em_unit_symbol(ast, c, inst->unit_buf, sizeof inst->unit_buf);
    /* the dimensions: between "<module>_<modifier>_" and "_<args>" of the symbol */
    char pre[NAME_CAP];
    size_t pl = inst->modifier.len ? k_mangle_decl(inst->module, inst->modifier, true, (keel_slice_char){ sizeof pre, pre })
                                   : k_mangle_module(inst->module, (keel_slice_char){ sizeof pre, pre });
    if (pl >= sizeof pre || pl > c->symbol_len || memcmp(c->symbol, pre, pl) != 0) return false;
    size_t lo = pl, hi = c->symbol_len;
    for (size_t k = c->argc; k-- > 0; ) {
        keel_slice_char a = inst->arg[k];
        if (hi < lo + a.len + 1 || memcmp(c->symbol + hi - a.len, a.ptr, a.len) != 0 || c->symbol[hi - a.len - 1] != '_')
            return false;
        hi -= a.len + 1;
    }
    for (size_t p = lo; p < hi; ) {
        if (c->symbol[p] != '_' || inst->dimc == K_EMIT_INSTANCE_ARGS) return false;
        size_t q = ++p;
        while (q < hi && c->symbol[q] >= '0' && c->symbol[q] <= '9') q++;
        if (q == p) return false;
        inst->dim[inst->dimc++] = (keel_slice_char){ q - p, (char *)c->symbol + p };
        p = q;
    }
    return true;
}

/* the instances a module places with `instance` (the closure entries it writes),
 * outside the base, whose bodies go to its .c */
static bool placed(const KClosure *c) {
    return c->origin == K_CLOSURE_WRITTEN && c->kind == K_CLOSURE_INSTANCE && c->module && !is_base(c->module->name);
}

bool k_em_places_instances(const KAst *ast) {
    bool any = false;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->kind == K_AST_INSTANCE) any = true;
    }
    return any;
}

bool k_em_supported_placement(const KAst *ast) {
    for (size_t i = 0; i < ast->nodes.len; i++) {
        KAstKind k = keel_buffer_KAstNode_ptr(&ast->nodes, i)->kind;
        if (k != K_AST_MODULE && k != K_AST_IMPORT && k != K_AST_INSTANCE) return false;
    }
    for (size_t i = 0; i < ast->closure.len; i++) {
        const KClosure *c = keel_buffer_KClosure_ptr(&ast->closure, i);
        if (!placed(c)) continue;
        KEmitInstance inst;
        if (!k_emit_instance_of(ast, c, &inst)) return false;
        Emit g = { .ast = c->module->ast, .module = inst.module, .inst = &inst, .ok = true, .cache_line = 1 };
        if (!instance_supported(&g)) return false;
    }
    return true;
}

/* The .c of a module that places instances (backend §4.4; linguagem §4.9): the
 * .h of each instance, then the bodies of its `pub` functions without `inline`,
 * each mapped to the generic source. The module's own lines have no C. */
void k_em_placement_c(Emit *e) {
    const KAst *ast = e->ast;
    for (size_t i = 0; i < ast->closure.len && e->ok; i++) {
        const KClosure *c = keel_buffer_KClosure_ptr(&ast->closure, i);
        if (!placed(c)) continue;
        char path[NAME_CAP];
        char unit[NAME_CAP];
        size_t ul = k_em_unit_symbol(ast, c, unit, sizeof unit);
        size_t pl = k_mangle_instance_path(c->module->name, ul ? (keel_slice_char){ ul, unit } : (keel_slice_char){ c->symbol_len, (char *)c->symbol }, ".h",
                                           (keel_slice_char){ sizeof path, path });
        if (pl > sizeof path) { e->ok = false; return; }
        raw(e, "#include \"");
        raw_slice(e, (keel_slice_char){ pl, path });
        raw(e, "\"\n");
    }
    for (size_t i = 0; i < ast->closure.len && e->ok; i++) {
        const KClosure *c = keel_buffer_KClosure_ptr(&ast->closure, i);
        if (!placed(c)) continue;
        KEmitInstance inst;
        if (!k_emit_instance_of(ast, c, &inst)) { e->ok = false; return; }
        char src_buf[NAME_CAP];
        if (inst.module.len + 2 > sizeof src_buf) { e->ok = false; return; }
        for (size_t k = 0; k < inst.module.len; k++) src_buf[k] = inst.module.ptr[k] == '.' ? '/' : inst.module.ptr[k];
        memcpy(src_buf + inst.module.len, ".k", 2);
        Emit g = { .ast = c->module->ast, .module = inst.module, .opt = e->opt, .inst = &inst, .ok = true, .cache_line = 1,
                   .src_path = { inst.module.len + 2, src_buf } };
        /* the same output, under the generic's path: every body starts with its #line */
        g.w = e->w;
        g.w.src_path = g.src_path;
        g.w.mapped = 0;
        for (size_t k = 0; k < g.ast->nodes.len && g.ok; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&g.ast->nodes, k);
            /* a `pub` body loses its visibility word; a `priv` one is `static` (backend §4.4) */
            if (k_em_generic_out(g.ast, n) && n->kind == K_AST_FUNCTION && !n->is_inline && !k_em_other_modifier(&g, n))
                emit_body(&g, n, !n->is_public);
        }
        g.w.src_path = e->w.src_path;
        g.w.mapped = 0;
        e->w = g.w;
        if (!g.ok) e->ok = false;
    }
}

size_t k_emit_instance(const KAst *generic, const KEmitInstance *inst, keel_slice_char src_path,
                       KEmitFile which, KEmitOptions opt, keel_slice_char out) {
    Emit e;
    e.ast = generic;
    e.module = inst->module;
    e.src_path = src_path;
    e.opt = opt;
    e.inst = inst;
    e.done = 0;
    e.cache_off = 0;
    e.cache_line = 1;
    e.ok = true;
    e.n = 0;
    e.block_ordinal = 0;
    e.scratch_n = 0;
    k_writer_init(&e.w, out, src_path, opt.line_on);
    if (which != K_EMIT_TYPE_H && which != K_EMIT_H && which != K_EMIT_C) return K_EMIT_UNSUPPORTED;
    if (!k_em_module_is_generic(generic) || (!inst->proto_fn.len && !instance_supported(&e))) return K_EMIT_UNSUPPORTED;
    if (which == K_EMIT_H) k_em_instance_h(&e);
    else if (which == K_EMIT_C) k_em_instance_c(&e);
    else emit_file(&e, which);
    if (!e.ok) return K_EMIT_UNSUPPORTED;
    return k_writer_len(&e.w);
}

bool k_emit_instance_redundant(const KAst *generic) {
    for (size_t i = 0; i < generic->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&generic->nodes, i);
        if (k_em_generic_out(generic, n) && n->kind == K_AST_FUNCTION && !n->is_inline) return false;
    }
    return true;
}

bool k_emit_generic_without_modifier(const KAst *generic, keel_slice_char *verb) {
    if (!k_em_module_is_generic(generic) || has_modifier(generic)) return false;
    for (size_t i = 0; i < generic->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&generic->nodes, i);
        if (n->kind != K_AST_FUNCTION || !n->is_public || !k_em_generic_out(generic, n)) continue;
        if (k_em_param_count(generic, n) != 1) continue;
        KToken t = k_em_tok(generic, n->name_first);
        *verb = (keel_slice_char){ t.len, (char *)t.ptr };
        return true;
    }
    return false;
}

bool k_emit_module_is_generic(const KAst *ast) { return k_em_module_is_generic(ast); }

bool k_emit_protocol_function(const KAst *ast, keel_slice_char name) {
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->kind != K_AST_FUNCTION || n->name_first >= n->name_end || !k_em_generic_out(ast, n)) continue;
        KToken t = k_em_tok(ast, n->name_first);
        if (t.len == name.len && memcmp(t.ptr, name.ptr, t.len) == 0 && k_em_over_protocol(ast, n)) return true;
    }
    return false;
}
