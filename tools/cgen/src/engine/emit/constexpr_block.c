/* engine/emit/constexpr_block.c — the block-scope `constexpr` of a function body
 * (emit_internal.h; codegen design E5, backend §9.2, rules 4 to 6). Under C23 the
 * declaration goes out verbatim and nothing here runs. Under C11 each
 * `constexpr T N = INIT;` of a body becomes `static const T keel__N_<k>__chk =
 * INIT; (void)keel__N_<k>__chk;` on its own line (INIT parenthesized when it has
 * more than one token), the uses of N up to the `}` of its block become
 * keel__N_<k>, a `#define` of that macro goes on the lines before, and a
 * `#undef` after the block. The ordinal <k> counts the block constexprs of the
 * function in token order and restarts at each function (backend §2.3, rule 2). */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define CB_NAME_MAX 200     /* the source name; the generated one must fit NAME_CAP */

/* the tokens of `constexpr T N = INIT;` (token indices) */
typedef struct { size_t name, eq, semi; } CbParts;

static bool is_ident(KToken t) {
    char c = t.len > 0 ? t.ptr[0] : '\0';
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* The function whose body holds token i: [body_first, body_end), and the
 * closing `}` (body_end) too when `closing`. NULL for any other token. */
static const KAstNode *body_of(const KAst *ast, size_t i, bool closing) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_FUNCTION || n->body_first == (size_t)-1 || n->body_end == (size_t)-1) continue;
        if (i >= n->body_first && (i < n->body_end || (closing && i == n->body_end))) return n;
    }
    return NULL;
}

/* `constexpr T N = INIT;` from the constexpr token c, in the form the C11 lowering
 * takes: a type (one or more tokens), an identifier, `=`, an initializer (one or
 * more tokens), `;`, all before `end` and on one line. */
static bool cb_parts(const KAst *ast, size_t end, size_t c, CbParts *p) {
    size_t eq = c + 1;
    while (eq < end && !k_em_text_is(k_em_tok(ast, eq), "=") && !k_em_text_is(k_em_tok(ast, eq), ";")) eq++;
    if (eq >= end || eq < c + 3 || !k_em_text_is(k_em_tok(ast, eq), "=")) return false;
    size_t semi = eq + 1;
    while (semi < end && !k_em_text_is(k_em_tok(ast, semi), ";")) semi++;
    if (semi >= end || semi < eq + 2) return false;
    KToken name = k_em_tok(ast, eq - 1);
    if (!is_ident(name) || name.len > CB_NAME_MAX) return false;
    size_t from = k_em_tok_off(ast, c), to = k_em_tok_end(ast, semi);
    for (size_t off = from; off < to; off++)
        if (ast->source.ptr[off] == '\n') return false;
    p->name = eq - 1;
    p->eq = eq;
    p->semi = semi;
    return true;
}

/* How many block constexprs of the function holding token c come before it, in
 * token order: the <k> of keel__N_<k> (backend §2.3, rule 2: a counter per
 * function, restarted at each one). */
static size_t cb_ordinal(const KAst *ast, size_t c) {
    size_t k = 0;
    for (size_t f = 0; f < ast->nodes.len; f++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, f);
        if (n->kind != K_AST_FUNCTION || n->body_first == (size_t)-1) continue;
        if (c < n->body_first || c >= n->body_end) continue;
        for (size_t i = n->body_first; i < c; i++)
            if (k_em_text_is(k_em_tok(ast, i), "constexpr")) k++;
        break;
    }
    return k;
}

/* keel__N_<k> for the constexpr at token c, written to out (cap bytes, length in
 * *len); false when c is not a constexpr of the lowered form or it does not fit. */
static bool cb_name(const KAst *ast, size_t end, size_t c, char *out, size_t cap, size_t *len) {
    CbParts p;
    if (!cb_parts(ast, end, c, &p)) return false;
    KToken n = k_em_tok(ast, p.name);
    char digits[24];
    size_t dn = 0, k = cb_ordinal(ast, c);
    do { digits[dn++] = (char)('0' + k % 10); k /= 10; } while (k);
    size_t total = 6 + n.len + 1 + dn;
    if (total > cap) return false;
    size_t m = 0;
    memcpy(out, "keel__", 6); m = 6;
    memcpy(out + m, n.ptr, n.len); m += n.len;
    out[m++] = '_';
    while (dn) out[m++] = digits[--dn];
    *len = m;
    return true;
}

/* Whether token u of fn's body is a use of a block constexpr, and which one (its
 * constexpr token in *decl): the nearest constexpr of that name whose block still
 * holds u (backend §9.2, rules 4 and 5). A name after `.` or `->` is a member,
 * and a token of its own statement is not a use. */
static bool cb_use(const KAst *ast, const KAstNode *fn, size_t u, size_t *decl) {
    if (u > 0 && (k_em_text_is(k_em_tok(ast, u - 1), ".") || k_em_text_is(k_em_tok(ast, u - 1), "->")))
        return false;
    KToken t = k_em_tok(ast, u);
    size_t depth = 0;
    for (size_t j = u; j > fn->body_first; ) {
        j--;
        if (k_em_text_is(k_em_tok(ast, j), "}")) depth++;
        else if (k_em_text_is(k_em_tok(ast, j), "{")) { if (depth > 0) depth--; }
        else if (depth == 0 && k_em_text_is(k_em_tok(ast, j), "constexpr")) {
            CbParts p;
            if (!cb_parts(ast, fn->body_end, j, &p)) continue;
            KToken nm = k_em_tok(ast, p.name);
            if (nm.len != t.len || memcmp(nm.ptr, t.ptr, t.len) != 0) continue;
            if (u <= p.semi) return false;      /* the name itself, or its own initializer */
            *decl = j;
            return true;
        }
    }
    return false;
}

/* The `#define` of the constexpr at token c, with the cast binding tighter than
 * an initializer of more than one token (backend §9.2, rule 1). */
static void put_define(Emit *e, const KAstNode *fn, size_t c) {
    const KAst *ast = e->ast;
    CbParts p;
    char name[NAME_CAP];
    size_t len;
    if (!cb_parts(ast, fn->body_end, c, &p) || !cb_name(ast, fn->body_end, c, name, sizeof name, &len)) {
        e->ok = false;
        return;
    }
    bool paren = p.semi > p.eq + 2;     /* more than one token of INIT */
    k_em_put_bytes(e, "#define ", 8);
    k_em_put_bytes(e, name, len);
    k_em_put_bytes(e, " ((", 3);
    k_em_put_range(e, c + 1, p.name);   /* T: the tokens between `constexpr` and N */
    k_em_put_bytes(e, ")", 1);
    if (paren) k_em_put_bytes(e, "(", 1);
    k_em_put_range(e, p.eq + 1, p.semi);
    if (paren) k_em_put_bytes(e, ")", 1);
    k_em_put_bytes(e, ")\n", 2);
}

/* The `#undef` of every block constexpr that the `}` at token `close` ends: those
 * directly in its block (not in a nested one). */
static void put_undefs(Emit *e, const KAstNode *fn, size_t close) {
    const KAst *ast = e->ast;
    size_t depth = 0, open = close;
    for (size_t j = close; j-- > fn->body_first - 1; ) {    /* the `{` that matches */
        if (k_em_text_is(k_em_tok(ast, j), "}")) depth++;
        else if (k_em_text_is(k_em_tok(ast, j), "{")) {
            if (depth == 0) { open = j; break; }
            depth--;
        }
    }
    depth = 0;
    for (size_t j = open + 1; j < close && e->ok; j++) {
        if (k_em_text_is(k_em_tok(ast, j), "{")) depth++;
        else if (k_em_text_is(k_em_tok(ast, j), "}")) depth--;
        else if (depth == 0 && k_em_text_is(k_em_tok(ast, j), "constexpr")) {
            char name[NAME_CAP];
            size_t len;
            if (!cb_name(ast, fn->body_end, j, name, sizeof name, &len)) { e->ok = false; return; }
            k_em_put_bytes(e, "#undef ", 7);
            k_em_put_bytes(e, name, len);
            k_em_put_bytes(e, "\n", 1);
        }
    }
}

/* the first token whose source offset is at least `off` (the tokens are in order) */
static size_t first_tok_from(const KAst *ast, size_t off) {
    size_t lo = 0, hi = ast->tokens.len;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (k_em_tok_off(ast, mid) < off) lo = mid + 1; else hi = mid;
    }
    return lo;
}

/* The lines written before (pre) or after (post) the .k line `ln`, bytes [a, b):
 * the `#undef`s of the `}` that opens or closes a line, as a `}` that is the first
 * token of its line sits before it (pre) and any other after it (post, `ln + 1`);
 * the `#define`s of the constexprs on the line go before it. One call writes one
 * text, so the lines of a line are a single #line run. */
static void lines_constblock(Emit *e, size_t ln, size_t a, size_t b, bool pre) {
    if (e->opt.c23 || !e->ok) return;
    const KAst *ast = e->ast;
    size_t ntok = ast->tokens.len;
    e->n = 0;
    for (size_t i = first_tok_from(ast, a); i < ntok && k_em_tok_off(ast, i) < b && e->ok; i++) {
        if (!k_em_text_is(k_em_tok(ast, i), "}")) continue;
        const KAstNode *fn = body_of(ast, i, true);
        if (!fn) continue;
        bool first = i == 0 || k_em_tok_end(ast, i - 1) <= k_em_line_start(e, k_em_tok_off(ast, i));
        if (first == pre) put_undefs(e, fn, i);
    }
    if (pre) {
        for (size_t i = first_tok_from(ast, a); i < ntok && k_em_tok_off(ast, i) < b && e->ok; i++) {
            if (!k_em_text_is(k_em_tok(ast, i), "constexpr")) continue;
            const KAstNode *fn = body_of(ast, i, false);
            if (fn) put_define(e, fn, i);
        }
    }
    if (e->ok && e->n > 0 && !k_emit_at(&e->w, pre ? ln : ln + 1, (keel_slice_char){ e->n, e->buf }))
        e->ok = false;
}

void k_em_edits_constblock(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    if (e->opt.c23) return;
    const KAst *ast = e->ast;
    for (size_t i = fn->body_first; i < fn->body_end && e->ok; i++) {
        size_t at = k_em_tok_off(ast, i);
        if (at < a || at >= b) continue;
        if (k_em_text_is(k_em_tok(ast, i), "constexpr")) {
            /* the statement: `static const`, the checked name, the check after `;` */
            CbParts p;
            char name[NAME_CAP];
            size_t len;
            if (!cb_parts(ast, fn->body_end, i, &p) || !cb_name(ast, fn->body_end, i, name, sizeof name, &len)) {
                e->ok = false;
                return;
            }
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok(ast, i).len,
                .name_tok = 0, .is_type = false, .text = "static const", .text_len = 12 });
            char *s = k_em_scratch(e, len + 5);
            if (!s) return;
            memcpy(s, name, len);
            memcpy(s + len, "__chk", 5);
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, p.name),
                .skip = k_em_tok(ast, p.name).len, .name_tok = 0, .is_type = false, .text = s, .text_len = len + 5 });
            if (p.semi > p.eq + 2) {   /* more than one token of INIT: parenthesized (backend §9.2, rule 2) */
                size_t open = k_em_tok_off(ast, p.eq + 1), close = k_em_tok_end(ast, p.semi - 1);
                if (open >= a && open < b)
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = open, .skip = 0,
                        .name_tok = 0, .is_type = false, .text = "(", .text_len = 1 });
                if (close >= a && close < b)
                    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = close, .skip = 0,
                        .name_tok = 0, .is_type = false, .text = ")", .text_len = 1 });
            }
            char *t = k_em_scratch(e, len + 13);
            if (!t) return;
            memcpy(t, " (void)", 7);
            memcpy(t + 7, name, len);
            memcpy(t + 7 + len, "__chk;", 6);
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, p.semi),
                .skip = 0, .name_tok = 0, .is_type = false, .text = t, .text_len = len + 13 });
        } else if (is_ident(k_em_tok(ast, i))) {
            size_t decl;
            if (!cb_use(ast, fn, i, &decl)) continue;
            /* a use: the token is copied by k_em_put_range, which renames it */
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TOKS, .at = at, .skip = k_em_tok(ast, i).len,
                .name_tok = 0, .is_type = false, .tok_a = i, .tok_b = i + 1 });
        }
    }
}

bool k_em_handles_constblock(const KAst *ast, const KIsland *is) {
    (void)ast; (void)is;
    return false;
}

void k_em_pre_lines_constblock(Emit *e, size_t ln, size_t a, size_t b) {
    lines_constblock(e, ln, a, b, true);
}

void k_em_post_lines_constblock(Emit *e, size_t ln, size_t a, size_t b) {
    lines_constblock(e, ln, a, b, false);
}

/* C23 takes the block constexpr verbatim, so every function is supported. C11
 * lowers the form `constexpr T N = INIT;` on one line; any other form makes the
 * call UNSUPPORTED (refused, not copied into invalid C). */
bool k_em_supported_constblock(const Emit *e, const KAstNode *fn) {
    if (e->opt.c23) return true;
    CbParts p;
    for (size_t i = fn->body_first; i < fn->body_end; i++)
        if (k_em_text_is(k_em_tok(e->ast, i), "constexpr") && !cb_parts(e->ast, fn->body_end, i, &p))
            return false;
    return true;
}

bool k_em_constblock_rename(const Emit *e, size_t tok, char *out, size_t cap, size_t *len) {
    if (e->opt.c23) return false;
    const KAstNode *fn = body_of(e->ast, tok, false);
    size_t decl;
    if (!fn || !is_ident(k_em_tok(e->ast, tok)) || !cb_use(e->ast, fn, tok, &decl)) return false;
    return cb_name(e->ast, fn->body_end, decl, out, cap, len);
}
