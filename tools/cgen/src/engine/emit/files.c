/* engine/emit/files.c — the files of a module: the .h, the .type.h, the .c and
 * the entry unit, each written from the nodes of the module (emit_internal.h;
 * backend §4.1, §4.3.2, §5.8; cgen design §8.3). k_emit_file is the call the
 * tool makes. No I/O, no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define EM_DASH "\xE2\x80\x94"  /* U+2014: the dash of every header comment */

#define skip k_em_skip

/* .type.h: the typedefs, the opaque C, the import_c lines and the body of
 * `extern_c [type_h]`, each with its own .k line (cgen design §8.3). A line
 * shared by two constructions (two typedefs) is written once. */
static void type_h_body(Emit *e) {
    const KAst *ast = e->ast;
    for (size_t i = 0; i < ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        size_t a, b;
        if (k_em_generic_out(ast, n)) continue;
        switch (n->kind) {
        case K_AST_IMPORT_C:
            a = k_em_line_start(e, k_em_first_off(e, n));
            if (a >= e->done) {
                k_em_emit_import(e, n, a);
                e->done = k_em_line_end(e, k_em_last_end(e, n));
            }
            break;
        case K_AST_CONSTEXPR:
            a = k_em_line_start(e, k_em_first_off(e, n));
            b = k_em_line_end(e, k_em_last_end(e, n));
            if (e->opt.c23) k_em_emit_range(e, a, b);
            else k_em_emit_constexpr_c11(e, n);
            e->done = b;
            break;
        case K_AST_TAGS:
        case K_AST_TYPE:
        case K_AST_OPAQUE:
            a = k_em_line_start(e, k_em_first_off(e, n));
            if (a < e->done) a = e->done;
            b = k_em_line_end(e, k_em_last_end(e, n) - 1);   /* a directive token ends past its newline */
            if (a < b) { k_em_emit_range(e, a, b); e->done = b; }
            break;
        case K_AST_EXTERN_C:
            if (!k_em_extern_c_type_h(ast, n)) break;   /* the plain and the priv forms: h_body and emit_c */
            if (k_em_extern_c_inline(ast, n)) {
                k_em_extern_c_text(e, n);
                e->done = k_em_line_end(e, k_em_last_end(e, n));
                break;
            }
            k_em_extern_c_lines(e, n, &a, &b);
            if (a < e->done) a = e->done;
            if (a < b) { k_em_emit_opaque_range(e, a, b); e->done = b; }
            break;
        default:
            break;
        }
    }
}

static void raw(Emit *e, const char *s);

/* A function of the .h: a `pub` one, and in an instance a `priv inline` one too,
 * which is `static inline` there under the instance's name (backend §4.3, rule 6) */
static bool header_function(const Emit *e, const KAstNode *n) {
    return n->kind == K_AST_FUNCTION && (n->is_public || (e->inst && n->is_inline));
}

/* a `pub inline` function, or in an instance a `priv inline` one: its body is in the .h */
static bool header_body(const Emit *e, const KAstNode *n) {
    return k_em_is_pub_inline(e->ast, n) || (e->inst && n->kind == K_AST_FUNCTION && n->is_inline);
}

static void h_body(Emit *e) {
    k_em_h_forward_decls(e);
    for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (skip(e, n)) continue;
        if (header_function(e, n)) k_em_emit_proto(e, n);
        else if (n->kind == K_AST_VARIABLE && n->is_public) {
            /* a declaration of several declarators: written at its first node only */
            if (i == 0 || keel_buffer_KAstNode_ptr(&e->ast->nodes, i - 1)->first != n->first) k_em_emit_extern(e, i);
        }
        else if (n->kind == K_AST_TYPE && k_em_is_extent(e->ast, n)) k_em_extent_protos(e, n);
        else if (n->kind == K_AST_EXTERN_C && !k_em_extern_c_type_h(e->ast, n) && !k_em_extern_c_priv(e->ast, n)) {
            /* a plain extern_c: its text, in its place among the prototypes (backend §4.3.2, rule 9) */
            if (k_em_extern_c_inline(e->ast, n)) { k_em_extern_c_text(e, n); continue; }
            size_t a, b;
            k_em_extern_c_lines(e, n, &a, &b);
            if (a < b) k_em_emit_opaque_range(e, a, b);
        }
    }
    /* then the bodies of the `pub inline` functions, each with its own .k lines */
    bool blank = false;
    for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        bool extent = n->kind == K_AST_TYPE && k_em_is_extent(e->ast, n);
        if (skip(e, n) || !(header_body(e, n) || extent)) continue;
        if (!blank) {
            raw(e, "\n");
            /* section 3: the .h of what the bodies call (backend §4.3.2, rule 3) */
            if (k_em_h_body_includes(e)) raw(e, "\n");
            blank = true;
        }
        if (extent) { k_em_extent_bodies(e, n); continue; }
        k_em_emit_range(e, k_em_line_start(e, k_em_first_off(e, n)), k_em_line_end(e, k_em_last_end(e, n)));
    }
}

static bool type_h_has_content(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (k_em_generic_out(e->ast, n)) continue;
        KAstKind k = n->kind;
        if (k == K_AST_IMPORT_C || k == K_AST_TYPE || k == K_AST_OPAQUE || k == K_AST_CONSTEXPR || k == K_AST_TAGS) return true;
        if (k == K_AST_EXTERN_C && k_em_extern_c_type_h(e->ast, n)) return true;
    }
    return false;
}

static bool h_has_content(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (skip(e, n)) continue;
        if (header_function(e, n) || (n->kind == K_AST_VARIABLE && n->is_public)) return true;
        if (n->kind == K_AST_TYPE && k_em_is_extent(e->ast, n)) return true;
        if (n->kind == K_AST_EXTERN_C && !k_em_extern_c_type_h(e->ast, n) && !k_em_extern_c_priv(e->ast, n)) return true;
    }
    return false;
}

/* ---- raw text and names ---- */

static void raw(Emit *e, const char *s) { k_emit_raw(&e->w, (keel_slice_char){ strlen(s), (char *)s }); }
static void raw_slice(Emit *e, keel_slice_char s) { k_emit_raw(&e->w, s); }

/* A name a mangle function wrote into buf; ok drops when it did not fit. */
static keel_slice_char fit(Emit *e, char *buf, size_t need) {
    if (need > NAME_CAP) e->ok = false;
    return (keel_slice_char){ need <= NAME_CAP ? need : 0, buf };
}

/* ---- the files ---- */

/* The frame of a header (backend §4.1; cgen design §8.3):
 *   R0 always a blank line after #define; R1 only after a block and before
 *   content; R5 before #endif unless the file is empty (no block, no content). */
static void emit_header(Emit *e, KEmitFile which) {
    bool type_h = which == K_EMIT_TYPE_H;
    bool c23 = e->opt.c23;
    bool keel_mod = k_em_is_keel(e->module);
    char path_buf[NAME_CAP], guard_buf[NAME_CAP], type_buf[NAME_CAP];
    char own_buf[NAME_CAP];
    const char *ext = type_h ? ".type.h" : ".h";
    keel_slice_char path = fit(e, path_buf, e->inst
        ? k_mangle_instance_path(e->inst->module, k_emit_unit(e->inst), ext, (keel_slice_char){ NAME_CAP, path_buf })
        : k_mangle_path(e->module, ext, (keel_slice_char){ NAME_CAP, path_buf }));
    keel_slice_char guard = fit(e, guard_buf, k_mangle_guard(path, (keel_slice_char){ NAME_CAP, guard_buf }));
    keel_slice_char type_path = fit(e, type_buf, e->inst
        ? k_mangle_instance_path(e->inst->module, k_emit_unit(e->inst), ".type.h", (keel_slice_char){ NAME_CAP, type_buf })
        : k_mangle_path(e->module, ".type.h", (keel_slice_char){ NAME_CAP, type_buf }));
    /* an instance reaches what is the generic's through the module's .type.h (backend §4.4.1, rule 4) */
    keel_slice_char own_type = fit(e, own_buf, k_mangle_path(e->module, ".type.h", (keel_slice_char){ NAME_CAP, own_buf }));
    if (!e->ok) return;

    bool has_content = type_h ? type_h_has_content(e) : h_has_content(e);
    /* the synthetic block (backend §4.1): under C11 the prelude's type header
     * includes stdbool and assert; the other modules include keel.type.h; the
     * C23 prelude has none */
    bool block = !(type_h && keel_mod) || !c23;

    raw(e, "/* ");
    raw_slice(e, path);
    raw(e, " " EM_DASH " generated from ");
    raw_slice(e, e->src_path);
    raw(e, c23 ? " by cgen, C23 profile. */\n" : " by cgen, C11 profile. */\n");
    raw(e, "#ifndef "); raw_slice(e, guard);
    raw(e, "\n#define "); raw_slice(e, guard);
    raw(e, "\n\n");
    if (type_h && keel_mod) {
        if (!c23) raw(e, "#include <stdbool.h>\n#include <assert.h>\n");
    } else if (type_h) {
        raw(e, "#include \"keel.type.h\"\n");
        if (!e->inst) k_em_type_h_includes(e);
    } else {
        raw(e, "#include \""); raw_slice(e, type_path); raw(e, "\"\n");
        if (e->inst && k_em_uses_share(e)) { raw(e, "#include \""); raw_slice(e, own_type); raw(e, "\"\n"); }
        k_em_h_type_includes(e);
    }
    if (block && has_content) raw(e, "\n");
    if (type_h) type_h_body(e);
    else h_body(e);
    if (block || has_content) raw(e, "\n");
    raw(e, "#endif /* "); raw_slice(e, guard); raw(e, " */\n");
}

void k_em_instance_h(Emit *e) { emit_header(e, K_EMIT_H); }

/* The .c of an instance compiled by itself (cgen design §2.8): its .h, then the
 * bodies of its `pub` functions without `inline`, each from the generic source */
void k_em_instance_c(Emit *e) {
    char c_buf[NAME_CAP], h_buf[NAME_CAP];
    keel_slice_char c_path = fit(e, c_buf, k_mangle_instance_path(e->inst->module, k_emit_unit(e->inst), ".c", (keel_slice_char){ NAME_CAP, c_buf }));
    keel_slice_char h_path = fit(e, h_buf, k_mangle_instance_path(e->inst->module, k_emit_unit(e->inst), ".h", (keel_slice_char){ NAME_CAP, h_buf }));
    if (!e->ok) return;
    raw(e, "/* ");
    raw_slice(e, c_path);
    raw(e, " " EM_DASH " generated from ");
    raw_slice(e, e->src_path);
    raw(e, e->opt.c23 ? " by cgen, C23 profile. */\n" : " by cgen, C11 profile. */\n");
    raw(e, "#include \""); raw_slice(e, h_path); raw(e, "\"\n");
    for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (skip(e, n) || !n->is_public || n->is_inline) continue;
        k_em_emit_range(e, k_em_line_start(e, k_em_first_off(e, n)), k_em_line_end(e, k_em_last_end(e, n)));
    }
}

/* The .c: every source line, in order. A line of a construction that does not
 * go to C is empty (backend §6, rule 3: the breaks stay); the rest is copied,
 * with the function's edits. */
static void emit_c(Emit *e) {
    char c_buf[NAME_CAP], h_buf[NAME_CAP];
    keel_slice_char c_path = fit(e, c_buf, k_mangle_path(e->module, ".c", (keel_slice_char){ NAME_CAP, c_buf }));
    keel_slice_char h_path = fit(e, h_buf, k_mangle_path(e->module, ".h", (keel_slice_char){ NAME_CAP, h_buf }));
    if (!e->ok) return;
    raw(e, "/* ");
    raw_slice(e, c_path);
    raw(e, " " EM_DASH " generated from ");
    raw_slice(e, e->src_path);
    raw(e, e->opt.c23 ? " by cgen, C23 profile. */\n" : " by cgen, C11 profile. */\n");
    raw(e, "#include \""); raw_slice(e, h_path); raw(e, "\"\n");
    if (k_em_places_instances(e->ast)) { k_em_placement_c(e); return; }
    k_em_use_includes(e);

    size_t len = e->ast->source.len;
    char nl[] = "\n";
    size_t written = 0;   /* the lines before this offset went out with an edit that spans them */
    for (size_t a = 0; a < len && e->ok; ) {
        size_t b = k_em_line_end(e, a);
        if (a < written) { a = b; continue; }
        bool empty = false, fn = false, opaque = false;
        const KAstNode *inl = NULL;   /* a one-line priv extern_c: its line is its text */
        for (size_t i = 0; i < e->ast->nodes.len; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (!k_em_overlaps(e, n, a, b)) continue;
            if (k_em_is_pub_inline(e->ast, n)) empty = true;   /* its body is in the .h */
            else if (n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE) fn = true;
            else if (n->kind == K_AST_EXTERN_C && k_em_extern_c_priv(e->ast, n)) {
                /* a priv extern_c: its text is the .c's (backend §4.3.2, rule 10); its braces are not.
                   Its content is opaque: no comment stripped, no name renamed (linguagem §2.1). */
                if (k_em_extern_c_inline(e->ast, n)) { inl = n; continue; }
                size_t ba, bb;
                k_em_extern_c_lines(e, n, &ba, &bb);
                if (a < ba || b > bb) empty = true;
                else opaque = true;
            }
            else empty = true;   /* every other kind in the slice has no C in the .c */
        }
        if (fn && empty) { e->ok = false; return; }
        if (inl) {
            /* the line of the `{` is the text, alone on it (supported() checks it) */
            if (inl->body_first >= inl->body_end) {
                if (!k_emit_at(&e->w, k_em_line_at(e, a), (keel_slice_char){ 1, nl })) e->ok = false;
            } else {
                k_em_extern_c_text(e, inl);
                if (k_em_spanned_end() > b) written = k_em_spanned_end();
            }
        } else if (empty) {
            if (!k_emit_at(&e->w, k_em_line_at(e, a), (keel_slice_char){ 1, nl })) e->ok = false;
        } else if (opaque) {
            k_em_emit_opaque_range(e, a, b);
        } else {
            k_em_emit_split(e, k_em_line_at(e, a), a, b);
            if (k_em_spanned_end() > b) written = k_em_spanned_end();
        }
        a = b;
    }
}

/* whether the module's `main` takes (argc, argv); `main()` and `main(void)` do not
 * (backend §5.8, rule 3; the parser has already refused any other signature) */
static bool main_takes_args(const Emit *e) {
    const KAst *ast = e->ast;
    if (!ast) return true;   /* no tree (a unit test): the general form */
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->kind != K_AST_FUNCTION || n->name_first >= n->name_end || !k_em_text_is(k_em_tok(ast, n->name_first), "main")) continue;
        size_t open = n->name_first + 1;
        return !(k_em_text_is(k_em_tok(ast, open + 1), ")") ||
                 (k_em_text_is(k_em_tok(ast, open + 1), "void") && k_em_text_is(k_em_tok(ast, open + 2), ")")));
    }
    return true;
}

/* main_<symbol>.c (backend §5.8): a fixed unit that calls the module's main */
static void emit_entry(Emit *e) {
    char entry_buf[NAME_CAP], h_buf[NAME_CAP], sym_buf[NAME_CAP];
    keel_slice_char entry = fit(e, entry_buf, k_mangle_entry(e->module, (keel_slice_char){ NAME_CAP, entry_buf }));
    keel_slice_char header = fit(e, h_buf, k_mangle_path(e->module, ".h", (keel_slice_char){ NAME_CAP, h_buf }));
    keel_slice_char main_name = { 4, (char *)"main" };
    keel_slice_char sym = fit(e, sym_buf, k_mangle_decl(e->module, main_name, false, (keel_slice_char){ NAME_CAP, sym_buf }));
    if (!e->ok) return;
    raw(e, "/* "); raw_slice(e, entry); raw(e, " " EM_DASH " entry unit, generated by --main ");
    raw_slice(e, e->module); raw(e, ". */\n");
    raw(e, "#include \""); raw_slice(e, header); raw(e, "\"\n");
    if (main_takes_args(e)) {
        raw(e, "int main(int argc, char **argv) { return "); raw_slice(e, sym);
        raw(e, "(argc, argv); }\n");
    } else {
        raw(e, "int main(void) { return "); raw_slice(e, sym);
        raw(e, "(); }\n");
    }
}

size_t k_emit_file(const KAst *ast, keel_slice_char module, keel_slice_char src_path,
                   KEmitFile which, KEmitOptions opt, keel_slice_char out) {
    Emit e;
    e.ast = ast;
    e.module = module;
    e.src_path = src_path;
    e.opt = opt;
    e.inst = NULL;
    e.done = 0;
    e.cache_off = 0;
    e.cache_line = 1;
    e.ok = true;
    e.n = 0;
    e.block_ordinal = 0;
    e.scratch_n = 0;
    k_writer_init(&e.w, out, src_path, opt.line_on);

    if (which == K_EMIT_ENTRY) {
        emit_entry(&e);
    } else {
        if (!k_em_supported(&e)) return K_EMIT_UNSUPPORTED;
        if (which == K_EMIT_C) emit_c(&e);
        else emit_header(&e, which);
    }
    if (!e.ok) return K_EMIT_UNSUPPORTED;
    return k_writer_len(&e.w);
}
