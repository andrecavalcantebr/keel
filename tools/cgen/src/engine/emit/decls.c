/* engine/emit/decls.c — the module-level declarations, each in its file form
 * (emit_internal.h; backend §4.1, §4.2, §4.3.2, §5.6, §9.2): the prototype of a
 * public function, the extern of a public variable, the import_c line, the C11
 * form of a constexpr, and the three forms of extern_c. No I/O, no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"

/* a `pub inline` function: its body goes to the .h as `static inline` (backend §4.3.2, rule 3) */
bool k_em_is_pub_inline(const KAst *ast, const KAstNode *n) {
    return n->kind == K_AST_FUNCTION && n->is_public && n->first + 1 < n->end &&
           k_em_text_is(k_em_tok(ast, n->first), "pub") && k_em_text_is(k_em_tok(ast, n->first + 1), "inline");
}

/* whether `n` is the first node of its declaration: the nodes of one declaration
 * with several declarators share its span, and only the first writes what they
 * share (a typedef's anonymous tag) */
static bool first_of_span(const KAst *ast, const KAstNode *n) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *d = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (d == n) return true;
        if (d->first == n->first) return false;
    }
    return true;
}

/* the edits of a declaration that node_edits applies first: visibility, tags, typedef,
 * extent, and the renames of its names. False when a limit was hit (node_edits returns). */
bool k_em_decl_edits(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    bool value = k_em_declares_value(n->kind);
    if (n->kind == K_AST_TYPE || n->kind == K_AST_TAGS || value) {
        size_t at = k_em_tok_off(ast, n->first);
        if (k_em_is_visibility(k_em_tok(ast, n->first)) && at >= a && at < b) {
            if (value && k_em_text_is(k_em_tok(ast, n->first), "priv")) {
                /* a priv function or variable is internal linkage in the .c */
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_STATIC, .at = at, .skip = k_em_tok(ast, n->first).len, .name_tok = 0, .is_type = false });
            } else if (k_em_is_pub_inline(ast, n)) {
                bool nodiscard = e->opt.c23 && k_em_nodiscard(ast, n);
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok(ast, n->first).len, .name_tok = 0, .is_type = false, .text = nodiscard ? "[[nodiscard]] static" : "static", .text_len = nodiscard ? 20 : 6 });
            } else {
                /* the keyword and the blanks after it */
                size_t skip = k_em_tok(ast, n->first).len;
                while (at + skip < b && (ast->source.ptr[at + skip] == ' ' || ast->source.ptr[at + skip] == '\t'))
                    skip++;
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at, .skip = skip, .name_tok = 0, .is_type = false });
            }
        }
    }
    if (n->kind == K_AST_TYPE && k_em_is_extent(ast, n)) {
        size_t before = *cnt;
        k_em_edits_extent(e, n, a, b, ed, cnt);
        k_em_sort_edits(ed, before, *cnt);
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
                k_em_text_is(k_em_tok(ast, k + 2), "{") && n->name_first < n->name_end && first_of_span(ast, n)) {
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
                if (e->inst && k_em_generic_out(ast, n)) {
                    KToken t = k_em_tok(ast, i);
                    bool member = i > 0 && (k_em_text_is(k_em_tok(ast, i - 1), ".") || k_em_text_is(k_em_tok(ast, i - 1), "->"));
                    keel_slice_char arg;
                    const char *txt = NULL;
                    size_t tl = 0;
                    bool selection = i > 0 && k_em_text_is(k_em_tok(ast, i - 1), "type");   /* roles.c drops it */
                    if (selection) continue;
                    if (!member && k_token_is_ident(t) && k_em_binder_arg(e, t, &arg)) {
                        /* a binder: its argument; by value, without the top-level
                         * qualifier (spec §4.3, rule 14): `const char *` but `char` */
                        bool signature = n->kind == K_AST_FUNCTION && n->body_first != (size_t)-1 && i < n->body_first;
                        if (signature && !(i + 1 < n->end && k_em_text_is(k_em_tok(ast, i + 1), "*"))) arg = k_em_unqualified(arg);
                        txt = arg.ptr; tl = arg.len;
                    } else if (!member && t.len == e->inst->modifier.len && memcmp(t.ptr, e->inst->modifier.ptr, t.len) == 0) {
                        txt = e->inst->symbol.ptr; tl = e->inst->symbol.len;   /* the modifier: the instance */
                    } else if (d && i == n->name_first && e->inst->proto_fn.len) {
                        /* <module>_<name>; roles.c adds the arity suffix and the concrete type */
                        char *w = k_em_scratch(e, NAME_CAP);
                        if (!w) return false;
                        size_t q = k_mangle_decl(e->module, (keel_slice_char){ t.len, (char *)t.ptr }, false, (keel_slice_char){ NAME_CAP, w });
                        if (q > NAME_CAP) { e->ok = false; return false; }
                        txt = w; tl = q;
                    } else if (d && (d->kind == K_AST_TYPE || d->kind == K_AST_CONSTEXPR) && k_em_generic_out(ast, d)) {
                        /* a type or a constant of the generic: the instance's (backend §4.3, rule 1), the
                         * unit's own typedef by the unit's name alone */
                        bool own = k_em_unit_typedef(e, d);
                        keel_slice_char pre = own ? k_emit_unit(e->inst) : e->inst->symbol;
                        char *w = k_em_scratch(e, pre.len + (own ? 0 : 1 + t.len));
                        if (!w) return false;
                        memcpy(w, pre.ptr, pre.len);
                        if (!own) { w[pre.len] = '_'; memcpy(w + pre.len + 1, t.ptr, t.len); }
                        txt = w; tl = pre.len + (own ? 0 : 1 + t.len);
                    } else if (d && d->kind == K_AST_FUNCTION && k_em_generic_out(ast, d)) {
                        /* a verb of the instance: <symbol>_<name>, and the rank suffix */
                        keel_slice_char rank = {0};
                        bool r = d == n && k_em_rank_suffix(e, d, &rank);
                        keel_slice_char pre = k_em_verb_on_modifier(e, d) ? e->inst->symbol : k_emit_unit(e->inst);
                        char *w = k_em_scratch(e, pre.len + 1 + t.len + (r ? 1 + rank.len : 0));
                        if (!w) return false;
                        size_t q = 0;
                        memcpy(w + q, pre.ptr, pre.len); q += pre.len;
                        w[q++] = '_';
                        memcpy(w + q, t.ptr, t.len); q += t.len;
                        if (r) { w[q++] = '_'; memcpy(w + q, rank.ptr, rank.len); q += rank.len; }
                        txt = w; tl = q;
                    }
                    if (txt) {
                        k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = t.len, .text = txt, .text_len = tl });
                        continue;
                    }
                }
                /* A local or a parameter keeps the name the .k wrote: the rename
                 * is the module's, and the parser says where one is by leaving an
                 * island. The declaration's own name is its own. */
                if (d && (i == n->name_first || k_em_named_island_at(ast, i)))
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_RENAME, .at = at, .skip = k_em_tok(ast, i).len, .name_tok = d->name_first, .is_type = d->kind == K_AST_TYPE || d->kind == K_AST_TAGS });
            }
        }
    }
    return true;
}

/* import_c <x>; -> #include <x> (backend §4.1; the prelude and hello use only
 * the <...> form, supported() refuses the rest). The header is one slice of
 * the source: its tokens are contiguous. */
void k_em_emit_import(Emit *e, const KAstNode *n, size_t a) {
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
void k_em_emit_proto(Emit *e, const KAstNode *n) {
    size_t a = k_em_first_off(e, n);
    size_t b = k_em_tok_end(e->ast, n->body_first - 2);
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    k_em_node_edits(e, n, a, b, ed, &cnt);
    k_em_comment_edits(e, a, b, ed, &cnt);
    if (!e->ok) return;
    e->n = 0;
    k_em_copy_edited(e, k_em_line_at(e, a), a, b, ed, cnt);
    k_em_put_bytes(e, ";\n", 2);
    if (e->ok && !k_emit_at(&e->w, k_em_line_at(e, a), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* A public variable's declaration for the .h (backend §4.1, rule 13): `extern`,
 * the declaration without the initializers; the definition stays in the .c.
 * `pub int x = 1, y = 2;` is one declaration of several declarators: the nodes
 * from `i` on that share its span are written together, once (`first` is the
 * one at `i`). Each `= INIT` goes, up to the `,` or `;` that ends it. */
void k_em_emit_extern(Emit *e, size_t i) {
    const KAst *ast = e->ast;
    const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
    size_t a = k_em_first_off(e, n), b = k_em_tok_off(ast, n->end - 1);   /* up to the `;` */
    while (b > a && (ast->source.ptr[b - 1] == ' ' || ast->source.ptr[b - 1] == '\t')) b--;
    Edit ed[MAX_EDITS];
    size_t cnt = 0;
    for (size_t j = i; j < ast->nodes.len; j++) {
        const KAstNode *d = keel_buffer_KAstNode_ptr(&ast->nodes, j);
        if (d->first != n->first) break;
        k_em_node_edits(e, d, a, b, ed, &cnt);
    }
    k_em_comment_edits(e, a, b, ed, &cnt);
    /* the initializers: from the end of the declarator's name to the `,` or `;` after the INIT */
    size_t cut[MAX_EDITS][2], ncut = 0;
    for (size_t p = n->first, depth = 0; p < n->end; p++) {
        KToken t = k_em_tok(ast, p);
        if (k_em_text_is(t, "(") || k_em_text_is(t, "[") || k_em_text_is(t, "{")) depth++;
        else if (k_em_text_is(t, ")") || k_em_text_is(t, "]") || k_em_text_is(t, "}")) depth--;
        else if (depth == 0 && k_em_text_is(t, "=")) {
            size_t q = p + 1, d2 = 0;
            for (; q < n->end; q++) {
                KToken u = k_em_tok(ast, q);
                if (k_em_text_is(u, "(") || k_em_text_is(u, "[") || k_em_text_is(u, "{")) d2++;
                else if (k_em_text_is(u, ")") || k_em_text_is(u, "]") || k_em_text_is(u, "}")) d2--;
                else if (d2 == 0 && (k_em_text_is(u, ",") || k_em_text_is(u, ";"))) break;
            }
            if (ncut >= MAX_EDITS) { e->ok = false; return; }
            size_t cs = k_em_tok_end(ast, p - 1), ce = k_em_tok_off(ast, q);
            if (ce > b) ce = b;
            cut[ncut][0] = cs; cut[ncut][1] = ce; ncut++;
            p = q;
        }
    }
    /* an edit inside an initializer goes with it */
    size_t kept = 0;
    for (size_t k = 0; k < cnt; k++) {
        bool inside = false;
        for (size_t c = 0; c < ncut && !inside; c++) inside = ed[k].at >= cut[c][0] && ed[k].at < cut[c][1];
        if (!inside) ed[kept++] = ed[k];
    }
    cnt = kept;
    for (size_t c = 0; c < ncut; c++) {
        if (cut[c][1] <= cut[c][0]) continue;
        k_em_push_edit(e, ed, &cnt, (Edit){ .kind = EDIT_DROP, .at = cut[c][0], .skip = cut[c][1] - cut[c][0], .name_tok = 0, .is_type = false });
    }
    k_em_sort_edits(ed, 0, cnt);
    k_em_drop_nested(ed, &cnt);
    if (!e->ok) return;
    e->n = 0;
    k_em_put_bytes(e, "extern ", 7);
    k_em_copy_edited(e, k_em_line_at(e, a), a, b, ed, cnt);
    k_em_put_bytes(e, ";\n", 2);
    if (e->ok && !k_emit_at(&e->w, k_em_line_at(e, a), (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* A module-level constexpr under C11 (backend §9.2, rules 1 and 2): the macro
 * and the checking object, both on the line of the constexpr. The cast binds
 * tighter than most operators, so an initializer of more than one token is
 * parenthesized; a single token (`4`) is not, as in golden 008. */
void k_em_emit_constexpr_c11(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0) + 1;   /* past `constexpr` */
    size_t ln = k_em_line_at(e, k_em_first_off(e, n));
    size_t ts = k_em_tok_off(ast, k), te = k_em_tok_off(ast, n->name_first);
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
    size_t n0 = e->n;
    k_em_put_range(e, n->name_first + 2, n->end - 1);   /* the INIT without its comments, its names renamed as uses (backend §9.2) */
    k_em_continue_from(e, n0);    /* each line break of the INIT continues the macro (backend §9.2, rule 1) */
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
    k_em_put_range(e, n->name_first + 2, n->end - 1);
    if (paren) k_em_put_bytes(e, ")", 1);
    /* a use of the object that generates no code (backend §9.2, rule 2) */
    k_em_put_bytes(e, "; _Static_assert(sizeof(", 24);
    k_em_put_mangled(e, n->name_first, false);
    k_em_put_bytes(e, "__chk) > 0, \"constexpr\");\n", 26);
    if (e->ok && !k_emit_at(&e->w, ln, (keel_slice_char){ e->n, e->buf })) e->ok = false;
}

/* ---- the header bodies ---- */

/* The three forms of extern_c (backend §4.3.2, rules 9 and 10): `[type_h]` goes
 * to the .type.h, a plain one to the .h in the order of the source, and `priv`
 * to the .c, where its text stays. */
bool k_em_extern_c_type_h(const KAst *ast, const KAstNode *n) {
    size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
    return k + 1 < n->end && k_em_text_is(k_em_tok(ast, k + 1), "[");
}

bool k_em_extern_c_priv(const KAst *ast, const KAstNode *n) {
    return k_em_text_is(k_em_tok(ast, n->first), "priv");
}

/* the lines between the braces of an extern_c: from the line after its `{` up
 * to the line of its `}` (not included) */
void k_em_extern_c_lines(const Emit *e, const KAstNode *n, size_t *a, size_t *b) {
    *a = k_em_line_end(e, k_em_tok_off(e->ast, n->body_first - 1));
    *b = k_em_line_start(e, k_em_tok_off(e->ast, n->body_end));
}

/* `extern_c { … }` with its braces on one line (backend §4.3.2, rule 9): its text
 * is a line of its own, the line of the `{`, and the blanks at the braces leave
 * it. No other construction may share that line (supported() checks it). */
bool k_em_extern_c_inline(const KAst *ast, const KAstNode *n) {
    return !memchr(ast->source.ptr + k_em_tok_off(ast, n->body_first - 1), '\n',
                   k_em_tok_off(ast, n->body_end) - k_em_tok_off(ast, n->body_first - 1));
}

/* the text of a one-line extern_c, on the line of its `{` (see extern_c_inline); an
 * empty one writes nothing */
void k_em_extern_c_text(Emit *e, const KAstNode *n) {
    if (n->body_first >= n->body_end) return;
    size_t a = k_em_tok_off(e->ast, n->body_first), b = k_em_tok_end(e->ast, n->body_end - 1);
    k_em_emit_line(e, k_em_line_at(e, a), a, b);
}
