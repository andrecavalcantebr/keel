/* engine/emit/roles.c — what a function signature says to keel and not to C
 * (emit_internal.h; codegen design E5: one file per construction). Lowers:
 *   - the roles `child`, `parent`, `invalidates` (spec §4.3): the word goes;
 *   - `[[nodiscard]]` on a function that returns a `child` pointer (backend §5.2,
 *     rule 5; C23 only, §9.1);
 *   - the erased `type T` parameter (backend §5.16, rule 1): two `size_t`
 *     parameters, `sizeof(T)` and `alignof(T)` in the body read them, and a
 *     `T *` return is `void *`;
 *   - the arity suffix of a function of the module declared in more than one
 *     arity (backend §2.1.1, rules 1, 2, 4 and 5);
 *   - the range check the backend writes at the top of a three-parameter
 *     `as_slice` (backend §5.17, rule 7a). */
#include <string.h>
#include "engine/emit/emit_internal.h"

/* appends `n` bytes of `s` at buf[*w] */
static void put(char *buf, size_t *w, const char *s, size_t n) { memcpy(buf + *w, s, n); *w += n; }

static keel_slice_char k_diag_text_len(const char *s, size_t n) { return (keel_slice_char){ n, (char *)s }; }

static bool is_role(KToken t) {
    return k_em_text_is(t, "child") || k_em_text_is(t, "parent") || k_em_text_is(t, "invalidates");
}

static size_t blanks_after(const KAst *ast, size_t i) {
    size_t p = k_em_tok_end(ast, i), lim = k_em_tok_off(ast, i + 1);
    const char *s = ast->source.ptr;
    while (p < lim && (s[p] == ' ' || s[p] == '\t')) p++;
    return p - k_em_tok_end(ast, i);
}

/* the `)` that closes the parameter list of `fn` */
static size_t params_close(const KAstNode *fn) { return fn->body_first - 2; }

/* the number of parameters written: a `(void)` or `()` has none; a `type T` is one */
static size_t param_count(const KAst *ast, const KAstNode *fn) {
    size_t open = fn->name_first + 1, close = params_close(fn);
    if (open >= close) return 0;
    if (close == open + 2 && k_em_text_is(k_em_tok(ast, open + 1), "void")) return 0;
    if (close == open + 1) return 0;
    size_t n = 1, depth = 0;
    for (size_t i = open + 1; i < close; i++) {
        KToken t = k_em_tok(ast, i);
        if (k_em_text_is(t, "(") || k_em_text_is(t, "[") || k_em_text_is(t, "{")) depth++;
        else if (k_em_text_is(t, ")") || k_em_text_is(t, "]") || k_em_text_is(t, "}")) depth--;
        else if (depth == 0 && k_em_text_is(t, ",")) n++;
    }
    return n;
}

static bool has_params(const KAst *ast, const KAstNode *fn);
size_t k_em_param_count(const KAst *ast, const KAstNode *fn) { return has_params(ast, fn) ? param_count(ast, fn) : 0; }

static bool has_params(const KAst *ast, const KAstNode *fn) {
    return fn->kind == K_AST_FUNCTION && fn->body_first != (size_t)-1 && fn->body_first >= 2 &&
           fn->name_first < fn->name_end && k_em_text_is(k_em_tok(ast, fn->name_first + 1), "(") &&
           k_em_text_is(k_em_tok(ast, params_close(fn)), ")");
}

size_t k_em_arity_suffix(const KAst *ast, const KAstNode *fn, char *out) {
    out[0] = '\0';
    if (!has_params(ast, fn)) return 0;
    size_t mine = param_count(ast, fn);
    if (mine < 2) return 0;   /* only the container: no suffix (rule 2) */
    bool overloaded = false;
    KToken name = k_em_tok(ast, fn->name_first);
    for (size_t k = 0; k < ast->nodes.len && !overloaded; k++) {
        const KAstNode *o = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (o == fn || !has_params(ast, o)) continue;
        KToken t = k_em_tok(ast, o->name_first);
        if (t.len == name.len && memcmp(t.ptr, name.ptr, t.len) == 0 && param_count(ast, o) != mine) overloaded = true;
    }
    if (!overloaded) return 0;
    size_t v = mine - 1, digits = 0;
    char rev[8];
    do { rev[digits++] = (char)('0' + v % 10); v /= 10; } while (v && digits < 7);
    for (size_t i = 0; i < digits; i++) out[i] = rev[digits - 1 - i];
    out[digits] = '\0';
    return digits;
}

bool k_em_nodiscard(const KAst *ast, const KAstNode *fn) {
    if (!has_params(ast, fn)) return false;
    bool child = false;
    for (size_t i = fn->first; i < fn->name_first; i++) {
        if (k_em_text_is(k_em_tok(ast, i), "child")) child = true;
        else if (child && k_em_text_is(k_em_tok(ast, i), "*")) return true;
    }
    return false;
}

void k_em_edits_roles(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    if (!has_params(ast, fn)) return;
    size_t close = params_close(fn);

    /* the roles */
    for (size_t i = fn->first; i < close; i++) {
        KToken t = k_em_tok(ast, i);
        size_t at = k_em_tok_off(ast, i);
        if (!is_role(t) || at < a || at >= b) continue;
        k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at, .skip = t.len + blanks_after(ast, i) });
    }

    /* the arity suffix, after the renamed name */
    char suf[8];
    keel_slice_char rank;
    size_t sl = e->inst && k_em_rank_suffix(e, fn, &rank) ? 0 : k_em_arity_suffix(ast, fn, suf);
    size_t after = k_em_tok_end(ast, fn->name_first);
    if (sl && after >= a && after <= b) {
        char *txt = k_em_scratch(e, sl);
        if (!txt) return;
        memcpy(txt, suf, sl);
        k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = after, .skip = 0, .text = txt, .text_len = sl });
    }

    /* `type T`: sized and aligned */
    for (size_t i = fn->name_first + 2; i + 1 < close + 1; i++) {
        if (!k_em_text_is(k_em_tok(ast, i), "type") || !k_token_is_ident(k_em_tok(ast, i + 1))) continue;
        KToken x = k_em_tok(ast, i + 1);
        size_t at = k_em_tok_off(ast, i);
        keel_slice_char sel;
        if (e->inst && k_em_binder_arg(e, x, &sel)) {
            /* selection (backend §5.16, rule 3): the parameter leaves the signature,
             * with the comma and the blanks after it */
            size_t to = k_em_tok_end(ast, i + 1);
            if (k_em_text_is(k_em_tok(ast, i + 2), ",")) to = k_em_tok_off(ast, i + 3);
            if (at >= a && at < b) k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at, .skip = to - at });
            continue;
        }
        if (at >= a && at < b) {
            char *txt = k_em_scratch(e, 2 * (13 + x.len + 6) + 2);
            if (!txt) return;
            size_t w = 0;
            put(txt, &w, "size_t keel__", 13); put(txt, &w, x.ptr, x.len); put(txt, &w, "_size, ", 7);
            put(txt, &w, "size_t keel__", 13); put(txt, &w, x.ptr, x.len); put(txt, &w, "_align", 6);
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok_end(ast, i + 1) - at, .text = txt, .text_len = w });
        }
        /* `X *` in the return type is `void *` */
        for (size_t r = fn->first; r + 1 < fn->name_first; r++) {
            KToken rt = k_em_tok(ast, r);
            size_t rat = k_em_tok_off(ast, r);
            if (rt.len == x.len && memcmp(rt.ptr, x.ptr, x.len) == 0 && k_em_text_is(k_em_tok(ast, r + 1), "*") && rat >= a && rat < b)
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = rat, .skip = rt.len, .text = "void", .text_len = 4 });
        }
        /* the body: `sizeof(X)` and `alignof(X)` */
        for (size_t j = fn->body_first; j + 3 < fn->end; j++) {
            KToken w0 = k_em_tok(ast, j), w2 = k_em_tok(ast, j + 2);
            bool sz = k_em_text_is(w0, "sizeof"), al = k_em_text_is(w0, "alignof") || k_em_text_is(w0, "_Alignof");
            if (!(sz || al) || !k_em_text_is(k_em_tok(ast, j + 1), "(") || !k_em_text_is(k_em_tok(ast, j + 3), ")")) continue;
            if (w2.len != x.len || memcmp(w2.ptr, x.ptr, x.len) != 0) continue;
            size_t jat = k_em_tok_off(ast, j);
            if (jat < a || jat >= b) continue;
            char *txt = k_em_scratch(e, 6 + x.len + 6);
            if (!txt) return;
            size_t w = 0;
            put(txt, &w, "keel__", 6); put(txt, &w, x.ptr, x.len);
            if (sz) put(txt, &w, "_size", 5); else put(txt, &w, "_align", 6);
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = jat, .skip = k_em_tok_end(ast, j + 3) - jat, .text = txt, .text_len = w });
        }
    }
}

/* the last identifier of parameter k (0-based) of fn: its name; and for an
 * `array ... [T N]` parameter, the binder N in *binder */
static bool param_name(const KAst *ast, const KAstNode *fn, size_t k, KToken *name, KToken *binder) {
    size_t open = fn->name_first + 1, close = params_close(fn), q = 0, depth = 0, start = open + 1;
    for (size_t i = open + 1; i <= close; i++) {
        KToken t = k_em_tok(ast, i);
        bool end = i == close || (depth == 0 && k_em_text_is(t, ","));
        if (!end) {
            if (k_em_text_is(t, "(") || k_em_text_is(t, "[")) depth++;
            else if (k_em_text_is(t, ")") || k_em_text_is(t, "]")) depth--;
            continue;
        }
        if (q++ == k) {
            binder->len = 0;
            name->len = 0;
            size_t d = 0;
            for (size_t j = start; j < i; j++) {
                KToken x = k_em_tok(ast, j);
                if (k_em_text_is(x, "[")) { d++; continue; }
                if (k_em_text_is(x, "]")) { d--; continue; }
                if (!k_token_is_ident(x)) continue;
                if (d == 0) *name = x;
                else if (j + 1 < i && (k_em_text_is(k_em_tok(ast, j + 1), "]") || k_em_text_is(k_em_tok(ast, j + 1), ",")) &&
                         k_em_text_is(k_em_tok(ast, start), "array") && binder->len == 0) *binder = x;
            }
            return name->len > 0;
        }
        start = i + 1;
    }
    return false;
}

void k_em_edits_range_check(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    if (!has_params(ast, fn) || !k_em_text_is(k_em_tok(ast, fn->name_first), "as_slice") || param_count(ast, fn) != 3) return;
    size_t brace = fn->body_first - 1;
    size_t at = k_em_tok_end(ast, brace);
    if (at < a || at >= b) return;
    const char *src = ast->source.ptr;
    size_t nl = at;
    while (nl < ast->source.len && (src[nl] == ' ' || src[nl] == '\t')) nl++;
    if (nl >= ast->source.len || src[nl] != '\n') return;   /* the `{` ends its line */
    KToken x, pa, pb, bx, unused;
    if (!param_name(ast, fn, 0, &x, &bx) || !param_name(ast, fn, 1, &pa, &unused) || !param_name(ast, fn, 2, &pb, &unused)) return;
    char len_buf[NAME_CAP];
    size_t ll;
    if (bx.len) {                                   /* over an `array`: the binder */
        if (bx.len + 6 > sizeof len_buf) return;
        memcpy(len_buf, "keel__", 6); memcpy(len_buf + 6, bx.ptr, bx.len); ll = 6 + bx.len;
    } else {                                        /* the `length` of the container's type */
        size_t pl;
        if (e->inst) {
            pl = e->inst->symbol.len;
            if (pl + 8 + 2 + x.len + 1 > sizeof len_buf) return;
            memcpy(len_buf, e->inst->symbol.ptr, pl);
            memcpy(len_buf + pl, "_length", 7); pl += 7;
        } else {
            pl = k_mangle_decl(e->module, k_diag_text_len("length", 6), false, (keel_slice_char){ sizeof len_buf, len_buf });
            if (pl + 2 + x.len + 1 > sizeof len_buf) return;
        }
        len_buf[pl++] = '(';
        memcpy(len_buf + pl, x.ptr, x.len); pl += x.len;
        len_buf[pl++] = ')';
        ll = pl;
    }
    static const char head[] = "\n    KEEL_CHECK(", tail[] = ", \"range-index-out-of-bounds\");";
    size_t n = sizeof head - 1 + pa.len + 4 + pb.len + 4 + pb.len + 4 + ll + sizeof tail - 1;
    char *txt = k_em_scratch(e, n);
    if (!txt) return;
    size_t w = 0;
    put(txt, &w, head, sizeof head - 1);
    put(txt, &w, pa.ptr, pa.len); put(txt, &w, " <= ", 4); put(txt, &w, pb.ptr, pb.len);
    put(txt, &w, " && ", 4); put(txt, &w, pb.ptr, pb.len); put(txt, &w, " <= ", 4);
    put(txt, &w, len_buf, ll);
    put(txt, &w, tail, sizeof tail - 1);
    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = nl, .skip = 0, .text = txt, .text_len = w });
}
