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
 * Refused (K_EMIT_UNSUPPORTED): a type, a constant or a `priv` function that
 * names a binder; a declaration that spans lines where one line is needed
 * (the modifier, a prototype); a comment inside a declaration; a generic
 * module with dimension or tag binders. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define EM_DASH "\xE2\x80\x94"  /* U+2014: the dash of every header comment */

static bool same_text(KToken a, KToken b) { return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0; }

static bool same_slice(KToken t, keel_slice_char s) { return t.len == s.len && memcmp(t.ptr, s.ptr, s.len) == 0; }

static void raw(Emit *e, const char *s) { k_emit_raw(&e->w, (keel_slice_char){ strlen(s), (char *)s }); }
static void raw_slice(Emit *e, keel_slice_char s) { k_emit_raw(&e->w, s); }

static void put_slice(Emit *e, keel_slice_char s) { k_em_put_bytes(e, s.ptr, s.len); }

/* the argument bound to the type binder spelled like `t`, or false */
static bool binder_arg(const Emit *e, KToken t, keel_slice_char *arg) {
    const KAst *ast = e->ast;
    const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    size_t k = 0;
    for (size_t j = m->type_first; j < m->type_end; j++) {
        KToken x = k_em_tok(ast, j);
        if (!k_token_is_ident(x)) continue;
        if (same_text(x, t)) {
            if (k >= e->inst->argc) return false;
            *arg = e->inst->arg[k];
            return true;
        }
        k++;
    }
    return false;
}

/* token `i` of the generic source, as the instance spells it */
static void put_token(Emit *e, size_t i) {
    const KAst *ast = e->ast;
    KToken t = k_em_tok(ast, i);
    bool member = i > 0 && (k_em_text_is(k_em_tok(ast, i - 1), ".") || k_em_text_is(k_em_tok(ast, i - 1), "->"));
    keel_slice_char arg;
    if (!member && k_token_is_ident(t)) {
        if (binder_arg(e, t, &arg)) { put_slice(e, arg); return; }
        if (same_slice(t, e->inst->modifier)) { put_slice(e, e->inst->symbol); return; }
        const KAstNode *d = k_em_declared_at(e, i);
        if (d) {
            if (k_em_generic_out(ast, d)) {
                put_slice(e, e->inst->symbol);
                k_em_put_bytes(e, "_", 1);
                k_em_put_bytes(e, t.ptr, t.len);
            } else {
                k_em_put_mangled(e, d->name_first, d->kind == K_AST_TYPE || d->kind == K_AST_TAGS);
            }
            return;
        }
    }
    k_em_put_bytes(e, t.ptr, t.len);
}

/* A gap between two tokens: blanks and breaks. A comment would carry its text
 * into a line of C: refused. */
static void put_gap(Emit *e, size_t g0, size_t g1) {
    const char *s = e->ast->source.ptr;
    for (size_t p = g0; p + 1 < g1; p++)
        if (s[p] == '/' && (s[p + 1] == '/' || s[p + 1] == '*')) { e->ok = false; return; }
    k_em_put_bytes(e, s + g0, g1 - g0);
}

/* The tokens of `n` in the source bytes [a, b), with the gaps as written. The
 * visibility word is dropped with the blanks after it (vis_static false), or
 * becomes `static` (true). */
static void put_span(Emit *e, const KAstNode *n, size_t a, size_t b, bool vis_static) {
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
        put_gap(e, pos, off);
        put_token(e, i);
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
    if (!one_line(e, k_em_first_off(e, n), b)) { e->ok = false; return; }
    e->n = 0;
    k_em_put_bytes(e, "typedef struct ", 15);
    put_slice(e, e->inst->symbol);
    k_em_put_bytes(e, " ", 1);
    put_span(e, n, a, b, false);
    k_em_put_bytes(e, " ", 1);
    put_slice(e, e->inst->symbol);
    k_em_put_bytes(e, ";\n", 2);
    flush(e, k_em_line_at(e, k_em_first_off(e, n)));
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
    put_span(e, n, a, b, n->is_inline);
    k_em_put_bytes(e, ";\n", 2);
    flush(e, k_em_line_at(e, a));
}

/* the body of a `pub inline` function: its source lines, one at a time */
static void emit_body(Emit *e, const KAstNode *n) {
    size_t a = k_em_line_start(e, k_em_first_off(e, n)), end = k_em_last_end(e, n);
    size_t ln = k_em_line_at(e, a);
    while (a < end && e->ok) {
        size_t next = k_em_line_end(e, a);
        e->n = 0;
        put_span(e, n, a, next, true);
        if (e->ok && (e->n == 0 || e->buf[e->n - 1] != '\n')) k_em_put_bytes(e, "\n", 1);
        flush(e, ln);
        a = next;
        ln++;
    }
}

/* whether the declaration is one the instance can carry */
static bool carried(const Emit *e, const KAstNode *n) {
    if (n->kind == K_AST_MODIFIER) return true;
    return is_function_with_body(e, n) && n->is_public;
}

static bool instance_supported(const Emit *e) {
    const KAst *ast = e->ast;
    const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    if (m->dim_first != m->dim_end || m->tags_first != m->tags_end) return false;
    if (e->inst->argc > K_EMIT_INSTANCE_ARGS) return false;
    bool modifier = false;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->first >= n->end || !k_em_generic_out(ast, n)) continue;
        if (!carried(e, n)) return false;
        if (n->kind == K_AST_MODIFIER) {
            if (!same_slice(k_em_tok(ast, n->name_first), e->inst->modifier)) return false;
            modifier = true;
        }
    }
    return modifier;
}

static bool has_function(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (k_em_generic_out(e->ast, n) && n->kind == K_AST_FUNCTION) return true;
    }
    return false;
}

static bool has_inline(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (k_em_generic_out(e->ast, n) && n->kind == K_AST_FUNCTION && n->is_inline) return true;
    }
    return false;
}

static void emit_file(Emit *e, KEmitFile which) {
    bool type_h = which == K_EMIT_TYPE_H;
    char path_buf[NAME_CAP], guard_buf[NAME_CAP], type_buf[NAME_CAP];
    size_t pn = k_mangle_instance_path(e->inst->module, e->inst->symbol, type_h ? ".type.h" : ".h", (keel_slice_char){ NAME_CAP, path_buf });
    size_t tn = k_mangle_instance_path(e->inst->module, e->inst->symbol, ".type.h", (keel_slice_char){ NAME_CAP, type_buf });
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
        raw(e, "#include \"keel.type.h\"\n\n");
        for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (n->kind == K_AST_MODIFIER) emit_modifier(e, n);
        }
    } else {
        raw(e, "#include \""); raw_slice(e, type_path); raw(e, "\"\n");
        if (has_function(e)) raw(e, "\n");
        for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (k_em_generic_out(e->ast, n) && n->kind == K_AST_FUNCTION) emit_proto(e, n);
        }
        if (has_inline(e)) raw(e, "\n");
        for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (k_em_generic_out(e->ast, n) && n->kind == K_AST_FUNCTION && n->is_inline) emit_body(e, n);
        }
    }
    raw(e, "\n#endif /* "); raw_slice(e, guard); raw(e, " */\n");
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
    if (which != K_EMIT_TYPE_H && which != K_EMIT_H) return K_EMIT_UNSUPPORTED;
    if (!k_em_module_is_generic(generic) || !instance_supported(&e)) return K_EMIT_UNSUPPORTED;
    emit_file(&e, which);
    if (!e.ok) return K_EMIT_UNSUPPORTED;
    return k_writer_len(&e.w);
}
