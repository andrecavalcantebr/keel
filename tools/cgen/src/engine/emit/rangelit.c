/* engine/emit/rangelit.c — the range literal outside a traversal (emit_internal.h;
 * codegen design E5: one file per construction; backend §5.7, rule 9).
 * `a..b` is `range.of(a, b)`: `range r = 2..5;` is `keel_range r = keel_range_of(2, 5);`.
 * The operands stay as written: the edits open the call at the left operand, turn
 * the `..` into the comma and close the call after the right operand.
 *
 * Left alone: a `..` inside brackets (an index, spec §4.5), one whose left operand
 * is introduced by `:` (the header of a `foreach` or `parallel`, where the bounds go
 * straight into the loop), an open end (`a..`, `..b`: the parser refuses them outside
 * an index), and operands over two lines. */
#include "engine/emit/emit_internal.h"

#define NO_TOKEN ((size_t)-1)

static bool word(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

static bool opens(const KAst *ast, size_t i) {
    return word(ast, i, "(") || word(ast, i, "[") || word(ast, i, "{");
}

static bool closes(const KAst *ast, size_t i) {
    return word(ast, i, ")") || word(ast, i, "]") || word(ast, i, "}");
}

/* whether the innermost bracket that holds token `i`, from `lo`, is a `[` */
static bool inside_square(const KAst *ast, size_t i, size_t lo) {
    size_t depth = 0;
    for (size_t j = i; j-- > lo; ) {
        if (closes(ast, j)) depth++;
        else if (opens(ast, j)) { if (!depth) return word(ast, j, "["); depth--; }
    }
    return false;
}

/* the first token of the left operand of the `..` at `dots`, or NO_TOKEN: the one
 * after the `=`, `(`, `,` or `return` that introduces it, at the same depth */
static size_t left_start(const KAst *ast, size_t dots, size_t lo) {
    size_t depth = 0, j = dots;
    while (j-- > lo) {
        if (closes(ast, j)) { depth++; continue; }
        if (opens(ast, j)) { if (!depth) return j + 1 < dots ? j + 1 : NO_TOKEN; depth--; continue; }
        if (depth) continue;
        if (word(ast, j, ":") || word(ast, j, ";") || word(ast, j, "?")) return NO_TOKEN;
        if (word(ast, j, "=") || word(ast, j, ",") || word(ast, j, "return")) return j + 1 < dots ? j + 1 : NO_TOKEN;
    }
    return NO_TOKEN;
}

/* the last token of the right operand of the `..` at `dots`, or NO_TOKEN */
static size_t right_end(const KAst *ast, size_t dots, size_t hi) {
    size_t depth = 0, j = dots + 1;
    for (; j < hi; j++) {
        if (opens(ast, j)) { depth++; continue; }
        if (closes(ast, j)) { if (!depth) break; depth--; continue; }
        if (!depth && (word(ast, j, ";") || word(ast, j, ","))) break;
    }
    return j > dots + 1 && j <= hi ? j - 1 : NO_TOKEN;
}

static bool one_line(const KAst *ast, size_t from, size_t to) {
    for (size_t p = from; p < to; p++)
        if (ast->source.ptr[p] == '\n') return false;
    return true;
}

/* the left and right operands of a literal this file lowers */
static bool read_literal(const KAst *ast, const KAstNode *fn, size_t dots, size_t *l, size_t *r) {
    if (!word(ast, dots, "..") || dots < fn->body_first || dots >= fn->body_end) return false;
    if (inside_square(ast, dots, fn->body_first)) return false;
    *l = left_start(ast, dots, fn->body_first);
    *r = right_end(ast, dots, fn->body_end);
    if (*l == NO_TOKEN || *r == NO_TOKEN) return false;
    return one_line(ast, k_em_tok_off(ast, *l), k_em_tok_end(ast, *r));
}

static void put_text(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, size_t at, size_t skip,
                     const char *text, size_t len) {
    if (at >= a && at < b)
        k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = skip, .text = text, .text_len = len });
}

void k_em_edits_rangelit(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t i = fn->body_first; i < fn->body_end; i++) {
        size_t l, r;
        if (!read_literal(ast, fn, i, &l, &r)) continue;
        put_text(e, ed, cnt, a, b, k_em_tok_off(ast, l), 0, "keel_range_of(", 14);
        put_text(e, ed, cnt, a, b, k_em_tok_off(ast, i), 2, ", ", 2);
        put_text(e, ed, cnt, a, b, k_em_tok_end(ast, r), 0, ")", 1);
    }
}
