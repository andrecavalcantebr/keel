/* engine/emit/array.c — the `array` construction of a function body (emit_internal.h;
 * codegen design E5: one file per construction). Lowers backend §5.1, rules 2 and 3:
 * the word `array` goes, its dimensions separated by commas become bracket pairs in
 * sequence (`array f32 m[2,3]` -> `f32 m[2][3]`), and the first dimension of a
 * parameter gets `static` (`i32 v[static 2][3]`).
 *
 * The same holds for an `array` variable at file scope, and for an `array` field of
 * a struct (`typedef struct { array i32 m[2, 3]; } Grid;`), which has no island of its own:
 * k_em_edits_struct_arrays finds its declarations by their tokens.
 *
 * A local or a module variable may have several declarators (`array i32 a[3], b[4];`
 * -> `i32 a[3], b[4];`, `priv array i32 p[3], q[2];` -> `static i32 p[3], q[2];`): the
 * word goes once, and the commas of each declarator's dimensions become `][`.
 * The forms this slice does not lower are refused (k_em_handles_array answers
 * false, and the module is refused with implementation-limit): several declarators
 * of one `array` field of a struct, and a parameter whose dimension has a binder
 * (`array T v[size_t n]`, backend §5.18), or a binder outside a parameter. */
#include <string.h>
#include "engine/emit/emit_internal.h"

/* The tokens of one `array` declarator, found from the word `array`. */
typedef struct {
    size_t arr;                 /* the word `array` */
    size_t lbr, rbr;            /* the dimension brackets: `[` and its `]` */
    bool param;                 /* in the signature, before the body (rule 3) */
    bool in_function;           /* in a function (a parameter or a local) */
    bool in_type;               /* in a type (a field of a struct): one declarator only */
    bool binder;                /* dimension 0 is `T name` (backend §5.18) */
    size_t bend;                /* binder: the token that ends dimension 0 (`,` or `]`) */
    size_t fn_end;              /* the end of the enclosing function */
} ArrayDecl;

static bool tok_is(const KAst *ast, size_t i, const char *s) {
    return k_em_text_is(k_em_tok(ast, i), s);
}

/* a word of the keel text: an identifier or a keyword (not a number) */
static bool tok_is_word(const KAst *ast, size_t i) {
    KToken t = k_em_tok(ast, i);
    if (t.len == 0) return false;
    char c = t.ptr[0];
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool tok_opens(const KAst *ast, size_t i) {
    return tok_is(ast, i, "[") || tok_is(ast, i, "(") || tok_is(ast, i, "{");
}

static bool tok_closes(const KAst *ast, size_t i) {
    return tok_is(ast, i, "]") || tok_is(ast, i, ")") || tok_is(ast, i, "}");
}

/* the function, file-scope variable or type whose tokens hold token `tok`, or NULL */
static const KAstNode *enclosing_fn(const KAst *ast, size_t tok) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if ((n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE || n->kind == K_AST_TYPE) &&
            n->first <= tok && tok < n->end) return n;
    }
    return NULL;
}

/* The number of blanks (spaces and tabs) between token i and token i + 1, the
 * ones that leave with the token; a newline or a comment ends the count. */
static size_t blanks_after(const KAst *ast, size_t i) {
    size_t p = k_em_tok_end(ast, i), lim = k_em_tok_off(ast, i + 1);
    const char *s = ast->source.ptr;
    while (p < lim && (s[p] == ' ' || s[p] == '\t')) p++;
    return p - k_em_tok_end(ast, i);
}

/* The token after the declarator that ends at token rbr: its initializer (`= INIT`)
 * is skipped, up to the `,` or `;` at depth 0 (or lim). */
static size_t after_declarator(const KAst *ast, size_t rbr, size_t lim) {
    if (!tok_is(ast, rbr + 1, "=")) return rbr + 1;
    int depth = 0;
    for (size_t i = rbr + 2; i < lim; i++) {
        if (tok_opens(ast, i)) depth++;
        else if (tok_closes(ast, i)) depth--;
        else if (depth == 0 && (tok_is(ast, i, ",") || tok_is(ast, i, ";"))) return i;
    }
    return lim;
}

/* The `]` that closes the `[` at token open, before lim (0 when none). */
static size_t close_of(const KAst *ast, size_t open, size_t lim) {
    int depth = 0;
    for (size_t i = open; i < lim; i++) {
        if (tok_opens(ast, i)) depth++;
        else if (tok_closes(ast, i) && --depth == 0) return i;
    }
    return 0;
}

/* Whether an earlier island of the file has the same anchor: the declarators of one
 * `array` are one island each, and the first one lowers all of them. */
static bool earlier_at_anchor(const KAst *ast, size_t k) {
    const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
    for (size_t j = 0; j < k; j++) {
        const KIsland *o = keel_buffer_KIsland_ptr(&ast->islands, j);
        if (o->kind == K_ISLAND_ARRAY && o->anchor == is->anchor) return true;
    }
    return false;
}

/* Whether the word `array` at token arr is a simple declarator this file lowers,
 * and where its tokens are. */
static bool array_decl(const KAst *ast, size_t arr, ArrayDecl *d) {
    if (arr >= ast->tokens.len || !tok_is(ast, arr, "array")) return false;
    const KAstNode *fn = enclosing_fn(ast, arr);
    if (!fn) return false;
    /* the first `[` after the word; a declaration without one, or one that ends
     * before it, is not a form this file knows */
    size_t lbr = arr + 1;
    for (;; lbr++) {
        if (lbr >= fn->end) return false;
        if (tok_is(ast, lbr, "[")) break;
        if (tok_is(ast, lbr, ";") || tok_is(ast, lbr, ",") || tok_is(ast, lbr, ")") ||
            tok_is(ast, lbr, "{") || tok_is(ast, lbr, "}")) return false;
    }
    /* the `]` that closes it */
    size_t rbr = 0;
    int depth = 0;
    for (size_t i = lbr; i < fn->end; i++) {
        if (tok_opens(ast, i)) depth++;
        else if (tok_closes(ast, i) && --depth == 0) { rbr = i; break; }
    }
    if (rbr == 0) return false;                     /* unclosed */
    d->arr = arr; d->lbr = lbr; d->rbr = rbr;
    d->param = fn->kind == K_AST_FUNCTION && arr < fn->body_first;
    d->in_function = fn->kind == K_AST_FUNCTION;
    d->in_type = fn->kind == K_AST_TYPE;
    /* no dimension: `array char s[] = "..."` keeps the `[]` for the initializer to size (spec §4.2);
       a parameter with none is not a form this file knows */
    if (rbr == lbr + 1 && d->param) return false;
    d->fn_end = fn->end;
    /* a binder: two words in a row in dimension 0 (backend §5.18); a parameter only */
    d->binder = false;
    size_t depth0 = 0;
    for (size_t i = lbr + 1; i < rbr; i++) {
        if (tok_opens(ast, i)) depth0++;
        else if (tok_closes(ast, i)) depth0--;
        else if (depth0 == 0 && tok_is(ast, i, ",")) break;
        if (tok_is_word(ast, i) && tok_is_word(ast, i + 1)) d->binder = true;
    }
    if (d->binder) {
        if (!d->param) return false;
        size_t i = lbr + 1;
        while (i < rbr && !tok_is(ast, i, ",")) i++;
        d->bend = i;
        if (d->bend - lbr < 3) return false;
        for (size_t j = lbr + 1; j < rbr; j++)   /* another binder among the rest */
            if (j > d->bend && tok_is_word(ast, j) && tok_is_word(ast, j + 1)) return false;
    }
    /* a struct field with a second declarator: refused (one each); a local's and a
     * module variable's declarators are read in turn */
    if (d->in_type && rbr + 1 < fn->end && tok_is(ast, rbr + 1, ",")) return false;
    return true;
}

/* The edits of one `array` declarator: the word goes, its commas become `][`, and a
 * parameter gets `static`, or a binder becomes `keel__n`. Positions outside [a, b) get none. */
static void declarator_edits(Emit *e, const ArrayDecl *d, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t at;

    /* 1: the word `array` and the blanks after it leave */
    at = k_em_tok_off(ast, d->arr);
    if (at >= a && at < b)
        k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at,
            .skip = k_em_tok(ast, d->arr).len + blanks_after(ast, d->arr) });

    if (d->binder) {
        /* backend §5.18: `v[T n, ...]` -> `v[][...]`, the binder after the parameter */
        size_t from = k_em_tok_end(ast, d->lbr), to = k_em_tok_off(ast, d->bend);
        bool more = d->bend != d->rbr;
        if (more) to = k_em_tok_off(ast, d->bend + 1);
        if (from >= a && from < b) {
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = from, .skip = to - from,
                .text = more ? "][" : "", .text_len = more ? 2 : 0 });
        }
        KToken nm = k_em_tok(ast, d->bend - 1);
        size_t ty0 = k_em_tok_off(ast, d->lbr + 1), ty1 = k_em_tok_end(ast, d->bend - 2);
        size_t n = 2 + (ty1 - ty0) + 1 + 6 + nm.len;
        char *txt = k_em_scratch(e, n);
        if (!txt) return;
        size_t w = 0;
        txt[w++] = ','; txt[w++] = ' ';
        memcpy(txt + w, ast->source.ptr + ty0, ty1 - ty0); w += ty1 - ty0;
        txt[w++] = ' ';
        memcpy(txt + w, "keel__", 6); w += 6;
        memcpy(txt + w, nm.ptr, nm.len); w += nm.len;
        at = k_em_tok_end(ast, d->rbr);
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0, .text = txt, .text_len = w });
        /* each `n` of the function after the parameter list is `keel__n` */
        for (size_t i = d->rbr + 1; i < d->fn_end; i++) {
            KToken t = k_em_tok(ast, i);
            if (t.len != nm.len || memcmp(t.ptr, nm.ptr, nm.len) != 0) continue;
            if (tok_is(ast, i - 1, ".") || tok_is(ast, i - 1, "->")) continue;
            size_t p = k_em_tok_off(ast, i);
            if (p < a || p >= b) continue;
            char *r = k_em_scratch(e, 6 + nm.len);
            if (!r) return;
            memcpy(r, "keel__", 6); memcpy(r + 6, nm.ptr, nm.len);
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p, .skip = t.len, .text = r, .text_len = 6 + nm.len });
        }
        return;
    }

    /* 2: each comma at the outer level of the dimensions becomes `][`, with
     * the blanks after it; a local's further declarators do the same */
    for (size_t lb = d->lbr, rb = d->rbr; ; ) {
        int depth = 0;
        for (size_t i = lb + 1; i < rb; i++) {
            if (tok_opens(ast, i)) { depth++; continue; }
            if (tok_closes(ast, i)) { depth--; continue; }
            if (depth != 0 || !tok_is(ast, i, ",")) continue;
            at = k_em_tok_off(ast, i);
            if (at < a || at >= b) continue;
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at,
                .skip = 1 + blanks_after(ast, i), .text = "][", .text_len = 2 });
        }
        /* the next declarator of a local: `, name[` */
        size_t nx = after_declarator(ast, rb, d->fn_end);
        if (d->param || d->in_type || nx + 3 >= d->fn_end || !tok_is(ast, nx, ",") ||
            !tok_is_word(ast, nx + 1) || !tok_is(ast, nx + 2, "[")) break;
        lb = nx + 2;
        rb = close_of(ast, lb, d->fn_end);
        if (rb == 0) break;
    }

    /* 3: a parameter's first dimension is `static` (backend §5.1, rule 3) */
    at = k_em_tok_end(ast, d->lbr);
    if (d->param && at >= a && at < b)
        k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0,
            .text = "static ", .text_len = 7 });
}

void k_em_edits_array(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->kind != K_ISLAND_ARRAY || is->anchor < fn->first || is->anchor >= fn->end) continue;
        if (earlier_at_anchor(ast, k)) continue;
        ArrayDecl d;
        if (array_decl(ast, is->anchor, &d)) declarator_edits(e, &d, a, b, ed, cnt);
    }
}

/* The `array` fields of the struct (or other type) n: they have no island, so the
 * word is found in the tokens of the type. */
void k_em_edits_struct_arrays(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t i = n->first; i < n->end; i++) {
        if (!tok_is(ast, i, "array")) continue;
        ArrayDecl d;
        if (array_decl(ast, i, &d) && !d.in_function) declarator_edits(e, &d, a, b, ed, cnt);
    }
}

bool k_em_supported_struct_arrays(const KAst *ast, const KAstNode *n) {
    for (size_t i = n->first; i < n->end; i++) {
        if (!tok_is(ast, i, "array")) continue;
        ArrayDecl d;
        if (!array_decl(ast, i, &d) || d.in_function) return false;
    }
    return true;
}

bool k_em_handles_array(const KAst *ast, const KIsland *is) {
    ArrayDecl d;
    return is->kind == K_ISLAND_ARRAY && array_decl(ast, is->anchor, &d);
}
