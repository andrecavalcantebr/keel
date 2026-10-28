/* engine/emit/foreach.c — the `foreach` construction of a function body (emit_internal.h;
 * codegen design E5: one file per construction; backend §5.7, §5.3 rule 9).
 * Lowered here: `foreach (T b : A..B)` over a literal range, with one binder and
 * a body that is a braced block or one simple statement. Every other `foreach`
 * (over a container, a range in a variable, two binders) stays refused. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define NO_TOKEN ((size_t)-1)

/* the parts of a `foreach` over a literal range, as token indices */
typedef struct {
    size_t type;        /* `auto`, or the written type of the binder */
    size_t binder;      /* the binder name */
    size_t colon;       /* `:` */
    size_t dots;        /* `..` */
    size_t close;       /* the `)` that ends the header */
    size_t body;        /* the body's first token: `{`, or the statement's first token */
    size_t last;        /* the body's last token: its `}`, or the statement's `;` */
    bool braced;
} Foreach;

static bool word(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

/* an identifier or a keyword-like word (a type name, `auto`, a binder) */
static bool word_like(const KAst *ast, size_t i) {
    if (i >= ast->tokens.len) return false;
    KToken t = k_em_tok(ast, i);
    char c = t.len ? t.ptr[0] : 0;
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* the token that closes the bracket at `open`, or NO_TOKEN */
static size_t closing(const KAst *ast, size_t open, const char *o, const char *c) {
    size_t depth = 0;
    for (size_t i = open; i < ast->tokens.len; i++) {
        if (word(ast, i, o)) depth++;
        else if (word(ast, i, c)) {
            if (--depth == 0) return i;
        }
    }
    return NO_TOKEN;
}

/* the statements that the simple-body form does not take (a body with one of
 * them needs its own braces, which this slice does not add) */
static bool is_statement_keyword(const KAst *ast, size_t i) {
    static const char *const words[] = { "if", "for", "while", "foreach", "switch", "do", "else" };
    for (size_t k = 0; k < sizeof words / sizeof words[0]; k++)
        if (word(ast, i, words[k])) return true;
    return false;
}

/* Reads the `foreach` at token `kw`: the header is exactly `( T b : A .. B )`,
 * with exactly one `..` and non-empty A and B, and the body is a braced block
 * or one simple statement. False for every other form. */
static bool analyze(const KAst *ast, size_t kw, Foreach *f) {
    if (!word(ast, kw, "foreach") || !word(ast, kw + 1, "(")) return false;
    size_t close = closing(ast, kw + 1, "(", ")");
    if (close == NO_TOKEN || close < kw + 6) return false;
    f->type = kw + 2;
    f->binder = kw + 3;
    f->colon = kw + 4;
    f->close = close;
    if (!word(ast, f->colon, ":") || !word_like(ast, f->type) || !word_like(ast, f->binder)) return false;

    size_t dots = NO_TOKEN, count = 0;
    for (size_t i = f->colon + 1; i < close; i++)
        if (word(ast, i, "..")) { dots = i; count++; }
    if (count != 1 || dots == f->colon + 1 || dots + 1 == close) return false;
    f->dots = dots;

    size_t b = close + 1;
    if (b >= ast->tokens.len) return false;
    if (word(ast, b, "{")) {
        size_t c = closing(ast, b, "{", "}");
        if (c == NO_TOKEN) return false;
        f->body = b;
        f->last = c;
        f->braced = true;
        return true;
    }
    size_t depth = 0;
    for (size_t i = b; i < ast->tokens.len; i++) {
        /* a `}` here is the function's own: the statement has no `;` before its end */
        if (word(ast, i, "{") || word(ast, i, "}") || is_statement_keyword(ast, i)) return false;
        if (word(ast, i, "(") || word(ast, i, "[")) depth++;
        else if (word(ast, i, ")") || word(ast, i, "]")) {
            if (depth == 0) return false;
            depth--;
        } else if (depth == 0 && word(ast, i, ";")) {
            f->body = b;
            f->last = i;
            f->braced = false;
            return true;
        }
    }
    return false;
}

/* the island of the `foreach` at token `kw`, or NULL */
static const KIsland *island_at(const KAst *ast, size_t kw) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (is->kind == K_ISLAND_FOREACH && is->anchor == kw) return is;
    }
    return NULL;
}

/* appends x when its position is in the line [a, b) */
static void push_in_line(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, Edit x) {
    if (x.at >= a && x.at < b) k_em_push_edit(e, ed, cnt, x);
}

void k_em_edits_foreach(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t kw = fn->body_first; kw < fn->body_end; kw++) {
        if (!word(ast, kw, "foreach")) continue;
        const KIsland *is = island_at(ast, kw);
        Foreach f;
        if (!is || !k_em_handles_foreach(ast, is) || !analyze(ast, kw, &f)) continue;
        /* the binder's name is written again in the C text; a binder that is also
         * a module name would be renamed in the copied tokens only, so refuse it */
        if (k_em_declared_at(e, f.binder)) { e->ok = false; return; }

        KToken name = k_em_tok(ast, f.binder);
        size_t nlen = name.len;
        /* `; i < ` and `; i++)`: the binder's name, in the scratch of this line */
        char *s = k_em_scratch(e, nlen + 5);
        if (!s) return;
        s[0] = ';'; s[1] = ' ';
        memcpy(s + 2, name.ptr, nlen);
        memcpy(s + 2 + nlen, " < ", 3);
        char *t = k_em_scratch(e, nlen + 5);
        if (!t) return;
        t[0] = ';'; t[1] = ' ';
        memcpy(t + 2, name.ptr, nlen);
        memcpy(t + 2 + nlen, "++)", 3);

        /* foreach -> `{ for` */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, kw),
            .skip = k_em_tok(ast, kw).len, .text = "{ for", .text_len = 5 });
        /* auto -> size_t; a written type stays */
        if (word(ast, f.type, "auto"))
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, f.type),
                .skip = k_em_tok(ast, f.type).len, .text = "size_t", .text_len = 6 });
        /* `:` -> `=` */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, f.colon),
            .skip = 1, .text = "=", .text_len = 1 });
        /* `..` -> `; i < ` */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, f.dots),
            .skip = 2, .text = s, .text_len = nlen + 5 });
        /* the `)` of the header -> `; i++)` */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, f.close),
            .skip = 1, .text = t, .text_len = nlen + 5 });
        if (f.braced) {
            /* the block's `}` closes the `for` as well */
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, f.last),
                .skip = 0, .text = " }", .text_len = 2 });
        } else {
            /* one statement: a block around it, closed after its `;` */
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, f.body),
                .skip = 0, .text = "{ ", .text_len = 2 });
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, f.last),
                .skip = 0, .text = " } }", .text_len = 4 });
        }
    }
}

bool k_em_handles_foreach(const KAst *ast, const KIsland *is) {
    if (is->kind != K_ISLAND_FOREACH) return false;
    /* the detail of a range is `<binder> : <A>..<B>`; over a container it has no `..` */
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    bool dots = false;
    for (size_t i = 0; i + 1 < is->text_len; i++)
        if (d[i] == '.' && d[i + 1] == '.') { dots = true; break; }
    if (!dots) return false;
    Foreach f;
    return analyze(ast, is->anchor, &f);
}
