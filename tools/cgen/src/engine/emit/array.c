/* engine/emit/array.c — the `array` construction of a function body (emit_internal.h;
 * codegen design E5: one file per construction). Lowers backend §5.1, rules 2 and 3:
 * the word `array` goes, its dimensions separated by commas become bracket pairs in
 * sequence (`array f32 m[2,3]` -> `f32 m[2][3]`), and the first dimension of a
 * parameter gets `static` (`i32 v[static 2][3]`).
 *
 * The same holds for an `array` variable at file scope.
 *
 * The forms this slice does not lower are refused (k_em_handles_array answers
 * false, and the module is refused with implementation-limit): a declaration with
 * more than one declarator (`array i32 a[3], b[4];`) and a parameter whose
 * dimension has a binder (`array T v[size_t n]`, backend §5.18). */
#include "engine/emit/emit_internal.h"

/* The tokens of one `array` declaration, found from its island. */
typedef struct {
    size_t arr;                 /* the word `array`: the island's anchor */
    size_t lbr, rbr;            /* the dimension brackets: `[` and its `]` */
    bool param;                 /* in the signature, before the body (rule 3) */
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

/* the function or file-scope variable whose tokens hold token `tok`, or NULL */
static const KAstNode *enclosing_fn(const KAst *ast, size_t tok) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if ((n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE) && n->first <= tok && tok < n->end) return n;
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

/* Whether island `is` is a simple `array` declaration this file lowers, and
 * where its tokens are. */
static bool array_decl(const KAst *ast, const KIsland *is, ArrayDecl *d) {
    if (is->kind != K_ISLAND_ARRAY || is->anchor >= ast->tokens.len) return false;
    size_t arr = is->anchor;
    if (!tok_is(ast, arr, "array")) return false;
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
    if (rbr == 0 || rbr == lbr + 1) return false;   /* unclosed, or no dimension */
    /* a binder: two words in a row among the dimensions (backend §5.18) */
    for (size_t i = lbr + 1; i < rbr; i++)
        if (tok_is_word(ast, i) && tok_is_word(ast, i + 1)) return false;
    d->arr = arr; d->lbr = lbr; d->rbr = rbr;
    d->param = fn->kind == K_AST_FUNCTION && arr < fn->body_first;
    /* a local with a second declarator: `array i32 a[3], b[4];` */
    if (!d->param && rbr + 1 < fn->end && tok_is(ast, rbr + 1, ",")) return false;
    return true;
}

void k_em_edits_array(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->anchor < fn->first || is->anchor >= fn->end) continue;
        ArrayDecl d;
        if (!array_decl(ast, is, &d)) continue;

        /* 1: the word `array` and the blanks after it leave */
        size_t at = k_em_tok_off(ast, d.arr);
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at,
                .skip = k_em_tok(ast, d.arr).len + blanks_after(ast, d.arr) });

        /* 2: each comma at the outer level of the dimensions becomes `][`, with
         * the blanks after it */
        int depth = 0;
        for (size_t i = d.lbr + 1; i < d.rbr; i++) {
            if (tok_opens(ast, i)) { depth++; continue; }
            if (tok_closes(ast, i)) { depth--; continue; }
            if (depth != 0 || !tok_is(ast, i, ",")) continue;
            at = k_em_tok_off(ast, i);
            if (at < a || at >= b) continue;
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at,
                .skip = 1 + blanks_after(ast, i), .text = "][", .text_len = 2 });
        }

        /* 3: a parameter's first dimension is `static` (backend §5.1, rule 3) */
        at = k_em_tok_end(ast, d.lbr);
        if (d.param && at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 0,
                .text = "static ", .text_len = 7 });
    }
}

bool k_em_handles_array(const KAst *ast, const KIsland *is) {
    ArrayDecl d;
    return array_decl(ast, is, &d);
}
