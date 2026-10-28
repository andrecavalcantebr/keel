/* engine/emit/emit.c — the module emitter (emit.h; backend §4.1, §4.3, §5.8,
 * §6; codegen-design §8). One call writes one generated file, in the slice
 * emit.h names. No I/O, no allocation: a source line that needs no change goes
 * to the writer as it is; a line that does (visibility dropped, a function
 * mangled, a static_assert spelled for C11, an import_c turned into #include)
 * is built in a fixed stack buffer. A buffer that is too small makes the call
 * UNSUPPORTED, never a truncated file. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define EM_DASH "\xE2\x80\x94"  /* U+2014: the dash of every header comment */

static void node_edits(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    bool value = k_em_declares_value(n->kind);
    if (n->kind == K_AST_TYPE || n->kind == K_AST_TAGS || value) {
        size_t at = k_em_tok_off(ast, n->first);
        if (k_em_is_visibility(k_em_tok(ast, n->first)) && at >= a && at < b) {
            if (value && k_em_text_is(k_em_tok(ast, n->first), "priv")) {
                /* a priv function or variable is internal linkage in the .c */
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_STATIC, .at = at, .skip = k_em_tok(ast, n->first).len, .name_tok = 0, .is_type = false });
            } else {
                /* the keyword and the blanks after it */
                size_t skip = k_em_tok(ast, n->first).len;
                while (at + skip < b && (ast->source.ptr[at + skip] == ' ' || ast->source.ptr[at + skip] == '\t'))
                    skip++;
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at, .skip = skip, .name_tok = 0, .is_type = false });
            }
        }
    }
    if (n->kind == K_AST_TAGS) {
        /* `tags N [A, B = 3];` -> `typedef enum M_N {M_N_A, M_N_B = 3} M_N;` (backend
         * §5.6, rule 1): the author's text stays; only the words around it change */
        size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
        size_t open = n->name_first + 1, close = n->end - 2;   /* `[` and `]` */
        if (k_em_tok_off(ast, k) >= a && k_em_tok_off(ast, k) < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, k), .skip = k_em_tok(ast, k).len, .name_tok = 0, .is_type = false, .text = "typedef enum", .text_len = 12 });
        if (k_em_tok_off(ast, n->name_first) >= a && k_em_tok_off(ast, n->name_first) < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_RENAME, .at = k_em_tok_off(ast, n->name_first), .skip = k_em_tok(ast, n->name_first).len, .name_tok = n->name_first, .is_type = true });
        if (k_em_tok_off(ast, open) >= a && k_em_tok_off(ast, open) < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, open), .skip = 1, .name_tok = 0, .is_type = false, .text = "{", .text_len = 1 });
        for (size_t i = open + 1; i < close; i++) {
            /* a tag name follows the `[` or a `,`; what follows an `=` is a value */
            if (!k_em_text_is(k_em_tok(ast, i - 1), "[") && !k_em_text_is(k_em_tok(ast, i - 1), ",")) continue;
            size_t at = k_em_tok_off(ast, i);
            if (at >= a && at < b)
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_ENUMERATOR, .at = at, .skip = k_em_tok(ast, i).len, .name_tok = n->name_first, .is_type = true });
        }
        if (k_em_tok_off(ast, close) >= a && k_em_tok_off(ast, close) < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_ENUM_CLOSE, .at = k_em_tok_off(ast, close), .skip = 1, .name_tok = n->name_first, .is_type = true });
    }
    if (!k_em_is_keel(e->module)) {   /* the prelude's names are not prefixed (backend §4.2) */
        if (n->kind == K_AST_TYPE) {
            /* only the declarator is a module name; the members are not */
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            if (k + 2 < n->end && k_em_text_is(k_em_tok(ast, k), "typedef") && k_em_text_is(k_em_tok(ast, k + 1), "struct") &&
                k_em_text_is(k_em_tok(ast, k + 2), "{") && n->name_first < n->name_end) {
                size_t at = k_em_tok_end(ast, k + 1);
                if (at >= a && at < b)
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TAG, .at = at, .skip = 0, .name_tok = n->name_first, .is_type = true });
            }
            for (size_t i = n->first; i < n->end; i++) {
                /* the declarator, and the module types the members use; a member
                 * or a value of the module is not renamed here */
                size_t at = k_em_tok_off(ast, i);
                if (at < a || at >= b) continue;
                const KAstNode *d = k_em_declared_at(e, i);
                if (d && (i == n->name_first || d->kind == K_AST_TYPE || d->kind == K_AST_TAGS))
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_RENAME, .at = at, .skip = k_em_tok(ast, i).len, .name_tok = d->name_first, .is_type = true });
            }
        } else if (value) {
            for (size_t i = n->first; i < n->end; i++) {
                size_t at = k_em_tok_off(ast, i);
                if (at < a || at >= b) continue;
                const KAstNode *d = k_em_declared_at(e, i);
                if (d) k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_RENAME, .at = at, .skip = k_em_tok(ast, i).len, .name_tok = d->name_first, .is_type = d->kind == K_AST_TYPE || d->kind == K_AST_TAGS });
            }
        }
    }
    /* the constructions of a function body (backend §5): each file adds its edits
     * in [a, b); the new ones are sorted in among the rename edits of this node */
    if (n->kind == K_AST_FUNCTION) {
        size_t before = *cnt;
        k_em_edits_array(e, n, a, b, ed, cnt);
        k_em_edits_foreach(e, n, a, b, ed, cnt);
        k_em_edits_constblock(e, n, a, b, ed, cnt);
        k_em_edits_defer(e, n, a, b, ed, cnt);
        k_em_edits_index(e, n, a, b, ed, cnt);
        k_em_sort_edits(ed, before, *cnt);
    }
    /* the prelude's static_assert in C11 (backend §4.2, §9): only the keyword changes */
    if (n->kind == K_AST_OPAQUE && !e->opt.c23 && k_em_text_is(k_em_tok(ast, n->first), "static_assert")) {
        size_t at = k_em_tok_off(ast, n->first);
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_ASSERT, .at = at, .skip = k_em_tok(ast, n->first).len, .name_tok = 0, .is_type = false });
    }
}

/* Comments leave the generated text and their lines stay (backend §6, rule 3):
 * a comment is dropped, and so are the blanks before it when it ended the
 * line. Tokens do not include comments, so they sit in the gaps between
 * tokens, which hold nothing else; that is where they are found. The edits
 * are merged into `ed` in source order. */
static void comment_edits(Emit *e, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    const char *s = ast->source.ptr;
    size_t ntok = ast->tokens.len;
    size_t lo = 0, hi = ntok;
    while (lo < hi) {   /* the first token that ends after a */
        size_t mid = (lo + hi) / 2;
        if (k_em_tok_end(ast, mid) > a) hi = mid; else lo = mid + 1;
    }
    size_t before = *cnt;
    size_t g0 = lo > 0 ? k_em_tok_end(ast, lo - 1) : 0;
    for (size_t i = lo; g0 < b; i++) {
        size_t g1 = i < ntok ? k_em_tok_off(ast, i) : ast->source.len;
        /* the scan starts at the gap's start, not at the line's: a block comment
         * that began on an earlier line still covers the start of this one */
        for (size_t p = g0; p < g1 && p < b; ) {
            size_t ce;
            if (s[p] == '/' && p + 1 < g1 && s[p + 1] == '/') {
                ce = p;
                while (ce < g1 && s[ce] != '\n') ce++;
            } else if (s[p] == '/' && p + 1 < g1 && s[p + 1] == '*') {
                ce = p + 2;
                while (ce + 1 < g1 && !(s[ce] == '*' && s[ce + 1] == '/')) ce++;
                ce = ce + 2 < g1 ? ce + 2 : g1;
            } else { p++; continue; }
            size_t cs0 = p, cs = cs0 < a ? a : cs0, end = ce < b ? ce : b;
            if (end > cs && s[end - 1] == '\n') end--;     /* the break stays */
            size_t rest = end;
            while (rest < b && (s[rest] == ' ' || s[rest] == '\t')) rest++;
            if (cs0 >= a && (rest >= b || s[rest] == '\n'))   /* it began here and ended the line */
                while (cs > g0 && cs > a && (s[cs - 1] == ' ' || s[cs - 1] == '\t')) cs--;
            if (end > cs) k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = cs, .skip = end - cs, .name_tok = 0, .is_type = false });
            p = ce > p ? ce : p + 1;
        }
        if (i >= ntok) break;
        g0 = k_em_tok_end(ast, i);
    }
    k_em_sort_edits(ed, before, *cnt);
}

/* Copies source [a, b) into buf, applying the edits (sorted by position). */
static void copy_edited(Emit *e, size_t a, size_t b, const Edit *ed, size_t cnt) {
    const char *s = e->ast->source.ptr;
    size_t pos = a;
    for (size_t k = 0; k < cnt && e->ok; k++) {
        k_em_put_bytes(e, s + pos, ed[k].at - pos);
        if (ed[k].kind == EDIT_RENAME) k_em_put_mangled(e, ed[k].name_tok, ed[k].is_type);
        else if (ed[k].kind == EDIT_ASSERT) k_em_put_bytes(e, "_Static_assert", 14);
        else if (ed[k].kind == EDIT_STATIC) k_em_put_bytes(e, "static", 6);
        else if (ed[k].kind == EDIT_TAG) { k_em_put_bytes(e, " ", 1); k_em_put_mangled(e, ed[k].name_tok, true); }
        else if (ed[k].kind == EDIT_TEXT) k_em_put_bytes(e, ed[k].text, ed[k].text_len);
        else if (ed[k].kind == EDIT_TOKS) k_em_put_range(e, ed[k].tok_a, ed[k].tok_b);
        else if (ed[k].kind == EDIT_ENUMERATOR) {
            k_em_put_mangled(e, ed[k].name_tok, true);
            k_em_put_bytes(e, "_", 1);
            k_em_put_bytes(e, s + ed[k].at, ed[k].skip);
        } else if (ed[k].kind == EDIT_ENUM_CLOSE) {
            k_em_put_bytes(e, "} ", 2);
            k_em_put_mangled(e, ed[k].name_tok, true);
        }
        pos = ed[k].at + ed[k].skip;
    }
    if (e->ok) k_em_put_bytes(e, s + pos, b - pos);
    e->scratch_n = 0;   /* the edits' text is spent */
}

/* Writes the source lines [a, b) as the .k line `ln` (one call: a changed
 * line is one text, and the writer owns its #line). */
static void emit_line(Emit *e, size_t ln, size_t a, size_t b) {
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (k_em_overlaps(e, n, a, b)) node_edits(e, n, a, b, ed, &cnt);
    }
    comment_edits(e, a, b, ed, &cnt);
    if (!e->ok) return;
    const char *s = e->ast->source.ptr;
    k_em_pre_lines_constblock(e, ln, a, b);
    if (cnt == 0 && b > a && s[b - 1] == '\n') {
        if (!k_emit_at(&e->w, ln, (keel_slice_char){ b - a, (char *)s + a })) e->ok = false;
    } else {
        e->n = 0;
        copy_edited(e, a, b, ed, cnt);
        if (e->n == 0 || e->buf[e->n - 1] != '\n') k_em_put_bytes(e, "\n", 1);
        if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
    }
    k_em_post_lines_constblock(e, ln, a, b);
}

/* the whole lines [a, b), each with its own .k number */
static void emit_range(Emit *e, size_t a, size_t b) {
    size_t ln = k_em_line_at(e, a);
    while (a < b && e->ok) {
        size_t next = k_em_line_end(e, a);
        emit_line(e, ln, a, next);
        a = next;
        ln++;
    }
}

/* import_c <x>; -> #include <x> (backend §4.1; the prelude and hello use only
 * the <...> form, supported() refuses the rest). The header is one slice of
 * the source: its tokens are contiguous. */
static void emit_import(Emit *e, const KAstNode *n, size_t a) {
    const KAst *ast = e->ast;
    size_t hs = k_em_tok_off(ast, n->first + 1), he = k_em_tok_end(ast, n->end - 2);
    e->n = 0;
    k_em_put_bytes(e, "#include ", 9);
    k_em_put_bytes(e, ast->source.ptr + hs, he - hs);
    k_em_put_bytes(e, "\n", 1);
    if (e->ok && !k_emit_at(&e->w, k_em_line_at(e, a), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* A public function's prototype (backend §4.1): the declaration up to the `)`
 * that closes the parameters, without the visibility, with the mangled name. */
static void emit_proto(Emit *e, const KAstNode *n) {
    size_t a = k_em_first_off(e, n);
    size_t b = k_em_tok_end(e->ast, n->body_first - 2);
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    node_edits(e, n, a, b, ed, &cnt);
    comment_edits(e, a, b, ed, &cnt);
    if (!e->ok) return;
    e->n = 0;
    copy_edited(e, a, b, ed, cnt);
    k_em_put_bytes(e, ";\n", 2);
    if (e->ok && !k_emit_at(&e->w, k_em_line_at(e, a), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* A public variable's declaration for the .h (backend §4.1, rule 13): `extern`,
 * the declaration without the initializer; the definition stays in the .c. */
static void emit_extern(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t a = k_em_first_off(e, n), b = k_em_last_end(e, n);
    for (size_t i = n->first, depth = 0; i < n->end; i++) {   /* the top-level `=` */
        KToken t = k_em_tok(ast, i);
        if (k_em_text_is(t, "(") || k_em_text_is(t, "[") || k_em_text_is(t, "{")) depth++;
        else if (k_em_text_is(t, ")") || k_em_text_is(t, "]") || k_em_text_is(t, "}")) depth--;
        else if (depth == 0 && (k_em_text_is(t, "=") || k_em_text_is(t, ";"))) { b = k_em_tok_off(ast, i); break; }
    }
    while (b > a && (ast->source.ptr[b - 1] == ' ' || ast->source.ptr[b - 1] == '\t')) b--;
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    node_edits(e, n, a, b, ed, &cnt);
    comment_edits(e, a, b, ed, &cnt);
    if (!e->ok) return;
    e->n = 0;
    k_em_put_bytes(e, "extern ", 7);
    copy_edited(e, a, b, ed, cnt);
    k_em_put_bytes(e, ";\n", 2);
    if (e->ok && !k_emit_at(&e->w, k_em_line_at(e, a), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* A module-level constexpr under C11 (backend §9.2, rules 1 and 2): the macro
 * and the checking object, both on the line of the constexpr. The cast binds
 * tighter than most operators, so an initializer of more than one token is
 * parenthesized; a single token (`4`) is not, as in golden 008. */
static void emit_constexpr_c11(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0) + 1;   /* past `constexpr` */
    size_t ln = k_em_line_at(e, k_em_first_off(e, n));
    size_t ts = k_em_tok_off(ast, k), te = k_em_tok_off(ast, n->name_first);
    size_t is = k_em_tok_off(ast, n->name_first + 2), ie = k_em_tok_end(ast, n->end - 2);
    while (te > ts && (ast->source.ptr[te - 1] == ' ' || ast->source.ptr[te - 1] == '\t')) te--;
    bool paren = n->end - 2 > n->name_first + 2;   /* more than one token */
    const char *s = ast->source.ptr;
    e->n = 0;
    k_em_put_bytes(e, "#define ", 8);
    k_em_put_mangled(e, n->name_first, false);
    k_em_put_bytes(e, " ((", 3);
    k_em_put_bytes(e, s + ts, te - ts);
    k_em_put_bytes(e, ")", 1);
    if (paren) k_em_put_bytes(e, "(", 1);
    k_em_put_bytes(e, s + is, ie - is);
    if (paren) k_em_put_bytes(e, ")", 1);
    k_em_put_bytes(e, ")\n", 2);
    if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
    e->n = 0;
    k_em_put_bytes(e, "static const ", 13);
    k_em_put_bytes(e, s + ts, te - ts);
    k_em_put_bytes(e, " ", 1);
    k_em_put_mangled(e, n->name_first, false);
    k_em_put_bytes(e, "__chk = ", 8);
    if (paren) k_em_put_bytes(e, "(", 1);
    k_em_put_bytes(e, s + is, ie - is);
    if (paren) k_em_put_bytes(e, ")", 1);
    /* a use of the object that generates no code (backend §9.2, rule 2) */
    k_em_put_bytes(e, "; _Static_assert(sizeof(", 24);
    k_em_put_mangled(e, n->name_first, false);
    k_em_put_bytes(e, "__chk) > 0, \"constexpr\");\n", 26);
    if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* ---- the header bodies ---- */

/* .type.h: the typedefs, the opaque C, the import_c lines and the body of
 * `extern_c [type_h]`, each with its own .k line (cgen design §8.3). A line
 * shared by two constructions (two typedefs) is written once. */
static void type_h_body(Emit *e) {
    const KAst *ast = e->ast;
    for (size_t i = 0; i < ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        size_t a, b;
        switch (n->kind) {
        case K_AST_IMPORT_C:
            a = k_em_line_start(e, k_em_first_off(e, n));
            if (a >= e->done) {
                emit_import(e, n, a);
                e->done = k_em_line_end(e, k_em_last_end(e, n));
            }
            break;
        case K_AST_CONSTEXPR:
            a = k_em_line_start(e, k_em_first_off(e, n));
            b = k_em_line_end(e, k_em_last_end(e, n));
            if (e->opt.c23) emit_range(e, a, b);
            else emit_constexpr_c11(e, n);
            e->done = b;
            break;
        case K_AST_TAGS:
        case K_AST_TYPE:
        case K_AST_OPAQUE:
            a = k_em_line_start(e, k_em_first_off(e, n));
            if (a < e->done) a = e->done;
            b = k_em_line_end(e, k_em_last_end(e, n));
            if (a < b) { emit_range(e, a, b); e->done = b; }
            break;
        case K_AST_EXTERN_C:
            /* the lines after the `{` line, up to the `}` line (not included) */
            a = k_em_line_end(e, k_em_tok_off(ast, n->body_first - 1));
            b = k_em_line_start(e, k_em_tok_off(ast, n->body_end));
            if (a < e->done) a = e->done;
            if (a < b) { emit_range(e, a, b); e->done = b; }
            break;
        default:
            break;
        }
    }
}

static void h_body(Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len && e->ok; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (n->kind == K_AST_FUNCTION && n->is_public) emit_proto(e, n);
        else if (n->kind == K_AST_VARIABLE && n->is_public) emit_extern(e, n);
    }
}

static bool type_h_has_content(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        KAstKind k = keel_buffer_KAstNode_ptr(&e->ast->nodes, i)->kind;
        if (k == K_AST_IMPORT_C || k == K_AST_TYPE || k == K_AST_OPAQUE || k == K_AST_EXTERN_C ||
            k == K_AST_CONSTEXPR || k == K_AST_TAGS) return true;
    }
    return false;
}
static bool h_has_content(const Emit *e) {
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if ((n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE) && n->is_public) return true;
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
    keel_slice_char path = fit(e, path_buf,
        k_mangle_path(e->module, type_h ? ".type.h" : ".h", (keel_slice_char){ NAME_CAP, path_buf }));
    keel_slice_char guard = fit(e, guard_buf, k_mangle_guard(path, (keel_slice_char){ NAME_CAP, guard_buf }));
    keel_slice_char type_path = fit(e, type_buf,
        k_mangle_path(e->module, ".type.h", (keel_slice_char){ NAME_CAP, type_buf }));
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
    } else {
        raw(e, "#include \""); raw_slice(e, type_path); raw(e, "\"\n");
    }
    if (block && has_content) raw(e, "\n");
    if (type_h) type_h_body(e);
    else h_body(e);
    if (block || has_content) raw(e, "\n");
    raw(e, "#endif /* "); raw_slice(e, guard); raw(e, " */\n");
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

    size_t len = e->ast->source.len;
    char nl[] = "\n";
    for (size_t a = 0; a < len && e->ok; ) {
        size_t b = k_em_line_end(e, a);
        bool empty = false, fn = false;
        for (size_t i = 0; i < e->ast->nodes.len; i++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
            if (!k_em_overlaps(e, n, a, b)) continue;
            if (n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE) fn = true;
            else empty = true;   /* every other kind in the slice has no C in the .c */
        }
        if (fn && empty) { e->ok = false; return; }
        if (empty) {
            if (!k_emit_at(&e->w, k_em_line_at(e, a), (keel_slice_char){ 1, nl })) e->ok = false;
        } else {
            emit_line(e, k_em_line_at(e, a), a, b);
        }
        a = b;
    }
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
    raw(e, "int main(int argc, char **argv) { return "); raw_slice(e, sym);
    raw(e, "(argc, argv); }\n");
}

/* Whether the islands of the module are the ones a rename covers: a type,
 * a name or a call whose detail is `<name>[/arity] → <symbol>` and whose name
 * is declared by this module. Any other island is a construction that needs
 * lowering (backend §5), which the slice does not have. */
static bool islands_supported(const KAst *ast) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->kind != K_ISLAND_TYPE && is->kind != K_ISLAND_NAME && is->kind != K_ISLAND_CALL) {
            /* a construction a file of its own lowers answers for its kind; the
             * rename check below does not apply to it */
            if (k_em_handles_array(ast, is) || k_em_handles_foreach(ast, is) ||
                k_em_handles_index(ast, is) || k_em_handles_constblock(ast, is) ||
                k_em_handles_defer(ast, is)) continue;
            return false;
        }
        const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
        size_t len = is->text_len, nm = 0;
        while (nm < len && d[nm] != '/' && d[nm] != ' ') nm++;
        bool arrow = false;
        for (size_t k = nm; k + 3 <= len; k++)
            if (memcmp(d + k, "\xE2\x86\x92", 3) == 0) { arrow = true; break; }
        if (!arrow) return false;
        bool declared = false;
        for (size_t k = 0; k < ast->nodes.len && !declared; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            if (n->name_first >= n->name_end) continue;
            KToken t = k_em_tok(ast, n->name_first);
            declared = t.len == nm && memcmp(t.ptr, d, nm) == 0;
        }
        if (!declared) return false;
    }
    return true;
}

/* Whether the slice of emit.h covers every node of the module. */
static bool supported(const Emit *e) {
    const KAst *ast = e->ast;
    if (!islands_supported(ast)) return false;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->first >= n->end) return false;
        switch (n->kind) {
        case K_AST_MODULE:
        case K_AST_PROTOCOL:
            break;
        case K_AST_OPAQUE:
            break;
        case K_AST_IMPORT_C:
            /* import_c <x>; with the <...> form only */
            if (n->end - n->first < 5) return false;
            if (k_em_tok(ast, n->first + 1).ptr[0] != '<') return false;
            if (!k_em_text_is(k_em_tok(ast, n->end - 2), ">") || !k_em_text_is(k_em_tok(ast, n->end - 1), ";")) return false;
            break;
        case K_AST_TYPE: {
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            if (k >= n->end || !k_em_text_is(k_em_tok(ast, k), "typedef")) return false;
            /* one declarator per typedef: a list shares the span of its nodes */
            if (i > 0 && keel_buffer_KAstNode_ptr(&ast->nodes, i - 1)->first == n->first) return false;
            break;
        }
        case K_AST_TAGS: {
            /* pub tags N [A, B = 3];  priv sends the enum to the .c, which is not in the slice;
             * a value is a number, optionally negative */
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            if (!n->is_public || n->name_first >= n->name_end || n->name_first != k + 1) return false;
            if (n->end - n->name_first < 4 || !k_em_text_is(k_em_tok(ast, n->name_first + 1), "[")) return false;
            for (size_t i = n->name_first + 2; i + 2 < n->end; i++) {
                if (!k_em_text_is(k_em_tok(ast, i), "=")) continue;
                size_t v = i + 1;
                if (k_em_text_is(k_em_tok(ast, v), "-")) v++;
                char c = k_em_tok(ast, v).ptr[0];
                if (c < '0' || c > '9') return false;
            }
            break;
        }
        case K_AST_CONSTEXPR: {
            /* [pub|priv] constexpr T NAME = INIT; on one line, INIT not naming the module */
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            if (n->name_first >= n->name_end || n->name_first + 2 >= n->end - 1 || n->name_first <= k + 1) return false;
            if (!k_em_text_is(k_em_tok(ast, k), "constexpr") || !k_em_text_is(k_em_tok(ast, n->name_first + 1), "=")) return false;
            for (size_t p = k_em_tok_off(ast, n->first); p < k_em_tok_end(ast, n->end - 1); p++)
                if (ast->source.ptr[p] == '\n') return false;
            break;
        }
        case K_AST_VARIABLE: {
            /* one declarator: no top-level comma */
            size_t depth = 0;
            for (size_t p = n->first; p < n->end; p++) {
                KToken t = k_em_tok(ast, p);
                if (k_em_text_is(t, "(") || k_em_text_is(t, "[") || k_em_text_is(t, "{")) depth++;
                else if (k_em_text_is(t, ")") || k_em_text_is(t, "]") || k_em_text_is(t, "}")) depth--;
                else if (depth == 0 && k_em_text_is(t, ",")) return false;
            }
            break;
        }
        case K_AST_EXTERN_C: {
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            if (k + 4 >= n->end) return false;
            if (!k_em_text_is(k_em_tok(ast, k), "extern_c") || !k_em_text_is(k_em_tok(ast, k + 1), "[") ||
                !k_em_text_is(k_em_tok(ast, k + 2), "type_h") || !k_em_text_is(k_em_tok(ast, k + 3), "]") ||
                !k_em_text_is(k_em_tok(ast, k + 4), "{")) return false;
            if (n->body_first == (size_t)-1 || n->body_end == (size_t)-1) return false;
            break;
        }
        case K_AST_FUNCTION:
            if (n->body_first == (size_t)-1 || n->body_first < 2) return false;
            if (!k_em_text_is(k_em_tok(ast, n->body_first - 1), "{") || !k_em_text_is(k_em_tok(ast, n->body_first - 2), ")"))
                return false;
            if (!k_em_supported_constblock(e, n)) return false;
            break;
        default:
            return false;
        }
    }
    return true;
}

size_t k_emit_file(const KAst *ast, keel_slice_char module, keel_slice_char src_path,
                   KEmitFile which, KEmitOptions opt, keel_slice_char out) {
    Emit e;
    e.ast = ast;
    e.module = module;
    e.src_path = src_path;
    e.opt = opt;
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
        if (!supported(&e)) return K_EMIT_UNSUPPORTED;
        if (which == K_EMIT_C) emit_c(&e);
        else emit_header(&e, which);
    }
    if (!e.ok) return K_EMIT_UNSUPPORTED;
    return k_writer_len(&e.w);
}
