/* engine/emit/foreach.c — the `foreach` construction of a function body (emit_internal.h;
 * codegen design E5: one file per construction; backend §5.7, §5.3 rule 9).
 * Lowered here: `foreach (T b : A..B)` over a literal range, with one binder;
 * `foreach (T v, I i : c)` over a container variable `c`, with two binders and
 * the value binder by value (§5.7, rules 1 to 5); `foreach (T v, I i : f(args))`
 * over a call of a function of this module that returns `X *` (§5.7, rule 1: the
 * call is evaluated once, into the temporary); and `foreach (T b : r)` and
 * `foreach (T v, I i : r)` over a `range` variable `r`, by value or by pointer
 * (§5.7, rule 6 and the two-binder form), and `foreach (T b : c)` over a variable `c` of
 * another type that declares `first`/`limit` (Countable, §5.1; rule 6). Each with a body
 * that is a braced block or one simple statement. Every other `foreach` (a value binder by
 * pointer, a call returning a value) stays refused. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/names.h"

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

/* the parts of a `foreach (T v, I i : c)` over a container variable */
typedef struct {
    size_t vtype_a, vtype_b;    /* the value binder's type, tokens [a, b) */
    size_t value;               /* the value binder */
    size_t itype_a, itype_b;    /* the index binder's type */
    size_t index;
    size_t cont;                /* the container variable */
    size_t close, body, last;
    bool braced;
    bool by_pointer;            /* the container is a pointer: no `&` */
    bool value_copy;            /* a `range` or a by-value modifier: the temporary is a copy, not a pointer */
    bool walk;                  /* `walk (T *p, C c : x)`: the cursor protocol, not the index one */
    bool ptr_binder;            /* `T *v`: the element's address (`ptr`), not a copy (`get`) */
    bool ptr_overloaded;        /* `ptr` is declared in more than one arity by the container's module: suffix `1` */
    bool array;                 /* the container is an `array` variable: C syntax, no verbs (spec §4.2) */
    size_t cont_end;            /* the container's last token, exclusive: cont + 1 for a variable */
    const char *dim; size_t dim_len; bool dim_binder;   /* its dimension 0 */
    const char *sym; size_t sym_len;   /* the container's instance */
} Over;

/* `foreach (T b : r)` over a `range` variable (one binder): the parts, as token indices */
typedef struct {
    size_t type;                /* `auto`, or the written type of the binder */
    size_t binder;
    size_t colon;
    size_t cont;                /* the range variable */
    size_t close, body, last;
    bool braced;
    bool by_pointer;            /* `range *r`: the temporaries read `*r` */
} RangeVar;

/* the body after the `)` at close: a braced block or one simple statement */
static bool read_body(const KAst *ast, size_t close, size_t *body, size_t *last, bool *braced) {
    size_t b = close + 1;
    if (b >= ast->tokens.len) return false;
    if (word(ast, b, "{")) {
        size_t c = closing(ast, b, "{", "}");
        if (c == NO_TOKEN) return false;
        *body = b; *last = c; *braced = true;
        return true;
    }
    size_t depth = 0;
    for (size_t i = b; i < ast->tokens.len; i++) {
        if (word(ast, i, "{") || word(ast, i, "}") || is_statement_keyword(ast, i)) return false;
        if (word(ast, i, "(") || word(ast, i, "[")) depth++;
        else if (word(ast, i, ")") || word(ast, i, "]")) {
            if (depth == 0) return false;
            depth--;
        } else if (depth == 0 && word(ast, i, ";")) {
            *body = b; *last = i; *braced = false;
            return true;
        }
    }
    return false;
}

/* whether the verb `ptr` of the modifier `name` as written (`vec`, `fv.vec`) is declared in
 * more than one arity by the module that declares the modifier (backend §2.1.1, rule 1).
 * *known is false when the name does not resolve to a declaring modifier. */
static bool ptr_overloaded_of(const KAst *ast, const char *name, size_t len, bool *known) {
    *known = false;
    if (!ast->symbols || len == 0) return false;
    const KSymbol *s = k_symbol_resolve(ast->symbols, (keel_slice_char){ len, (char *)name });
    if (!s || s->kind != K_SYM_MODIFIER || !s->origin || !s->origin->ast) return false;
    *known = true;
    return k_em_name_overloaded(s->origin->ast, "ptr", 3);
}

/* the name written at token `at`, with its `.` and second word when it is qualified
 * (`fv.vec`), into out: its length, or 0 when it does not fit */
static size_t written_name(const KAst *ast, size_t at, char *out, size_t cap) {
    KToken t = k_em_tok(ast, at);
    if (t.len > cap) return 0;
    memcpy(out, t.ptr, t.len);
    size_t n = t.len;
    if (word(ast, at + 1, ".") && word_like(ast, at + 2)) {
        KToken u = k_em_tok(ast, at + 2);
        if (n + 1 + u.len > cap) return 0;
        out[n++] = '.';
        memcpy(out + n, u.ptr, u.len);
        n += u.len;
    }
    return n;
}

/* the symbol and the pointer-ness of the call `f(args)` in [a, close), a call of a function
 * of this module that returns `X *` (the return type's own island gives `X`'s symbol): the
 * temporary takes the result (§5.7, rule 1). A value return, an unknown callee, and an
 * argument with `[` or `..` (its own islands are not copied) are refused. */
static bool analyze_call_over(const KAst *ast, size_t a, size_t close, Over *o) {
    for (size_t i = a + 2; i < close - 1; i++)
        if (word(ast, i, "[") || word(ast, i, "..")) return false;
    const KAstNode *fn = NULL;
    KToken name = k_em_tok(ast, a);
    for (size_t k = 0; k < ast->nodes.len && !fn; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_FUNCTION || n->name_first >= n->name_end || n->name_first < 2) continue;
        KToken t = k_em_tok(ast, n->name_first);
        if (t.len == name.len && memcmp(t.ptr, name.ptr, t.len) == 0) fn = n;
    }
    if (!fn) return false;
    size_t n = fn->name_first;
    if (!word(ast, n - 1, "*") || word(ast, n - 2, "*")) return false;   /* exactly one `*`: `X *` */
    size_t s = fn->first;
    while (s < n && (word(ast, s, "pub") || word(ast, s, "priv") || word(ast, s, "inline") ||
                     word(ast, s, "static") || word(ast, s, "extern"))) s++;
    if (!k_em_type_island_at(ast, s, &o->sym, &o->sym_len)) return false;
    /* the return's modifier written at s: the `ptr` suffix is its module's (§2.1.1, rule 1) */
    o->ptr_overloaded = false;
    if (o->ptr_binder) {
        char nm[128];
        bool known = false;
        size_t nl = written_name(ast, s, nm, sizeof nm);
        o->ptr_overloaded = nl && ptr_overloaded_of(ast, nm, nl, &known);
        if (!known) return false;
    }
    o->cont = a;
    o->cont_end = close;
    o->array = false;
    o->by_pointer = true;      /* the temporary is a pointer: the call's result, no `&` */
    o->value_copy = false;
    return true;
}

/* `( T v , I i : c )` with the container's type in the island's detail:
 * `v, i : c (buffer i32)`; or the container is a call (analyze_call_over) */
static bool analyze_over(const KAst *ast, const KIsland *is, size_t kw, const KAstNode *fn, Over *o) {
    o->walk = word(ast, kw, "walk");
    if ((!word(ast, kw, "foreach") && !o->walk) || !word(ast, kw + 1, "(")) return false;
    size_t close = closing(ast, kw + 1, "(", ")");
    if (close == NO_TOKEN) return false;
    size_t comma = NO_TOKEN, colon = NO_TOKEN;
    for (size_t i = kw + 2; i < close; i++) {
        if (word(ast, i, ",") && comma == NO_TOKEN) comma = i;
        else if (word(ast, i, ":")) colon = i;
        else if (word(ast, i, "..") || (colon == NO_TOKEN && (word(ast, i, "(") || word(ast, i, "[")))) return false;
    }
    bool call = colon != NO_TOKEN && word_like(ast, colon + 1) && word(ast, colon + 2, "(") &&
                closing(ast, colon + 2, "(", ")") == close - 1;
    if (comma == NO_TOKEN || colon == NO_TOKEN || comma < kw + 4 || colon < comma + 3 || (colon + 2 != close && !call)) return false;
    o->vtype_a = kw + 2; o->vtype_b = comma - 1; o->value = comma - 1;
    o->itype_a = comma + 1; o->itype_b = colon - 1; o->index = colon - 1;
    o->cont = colon + 1;
    o->cont_end = o->cont + 1;
    o->close = close;
    if (!word_like(ast, o->value) || !word_like(ast, o->index) || !word_like(ast, o->cont)) return false;
    o->ptr_binder = false;
    o->ptr_overloaded = false;
    for (size_t i = o->vtype_a; i < o->vtype_b && !o->walk; i++) if (word(ast, i, "*")) o->ptr_binder = true;
    if (!read_body(ast, close, &o->body, &o->last, &o->braced)) return false;
    if (call) return !o->walk && analyze_call_over(ast, colon + 1, close, o);
    /* over an `array` variable the detail has no container type: `sl, i : steps` */
    {
        const char *dd = keel_buffer_char_ptr(&ast->island_text, is->text);
        o->array = false;
        if (!o->walk && is->text_len && dd[is->text_len - 1] != ')') {
            o->array = k_em_array_dim0(ast, o->cont, &o->dim, &o->dim_len, &o->dim_binder);
            if (o->array) { o->value_copy = false; o->by_pointer = false; return true; }
            return false;
        }
    }
    /* the container's type, from the detail: `... (<type>)` */
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, open = len;
    while (open > 0 && d[open - 1] != '(') open--;
    if (open == 0 || len < open + 2 || d[len - 1] != ')') return false;
    if (!k_em_type_symbol(ast, d + open, len - 1 - open, &o->sym, &o->sym_len)) return false;
    /* the written modifier: the text up to its first space (`fv.vec i32` gives `fv.vec`) */
    if (o->ptr_binder) {
        size_t nl = 0;
        while (nl < len - 1 - open && d[open + nl] != ' ') nl++;
        bool known = false;
        o->ptr_overloaded = ptr_overloaded_of(ast, d + open, nl, &known);
        if (!known) return false;
    }
    /* a range has no storage to point to: its temporary is a copy (the golden 015) */
    o->value_copy = (len - 1 - open == 5 && memcmp(d + open, "range", 5) == 0) || k_em_type_by_value(ast, o->sym, o->sym_len);
    /* by value or by pointer: the declaration `<type> c` or `<type> *c` in the function */
    KToken c = k_em_tok(ast, o->cont);
    bool found = false;
    for (size_t i = fn->first + 1; i < kw && !found; i++) {
        KToken t = k_em_tok(ast, i);
        if (t.len != c.len || memcmp(t.ptr, c.ptr, c.len) != 0) continue;
        if (word(ast, i - 1, "*")) { o->by_pointer = true; found = true; }
        else if (word_like(ast, i - 1) && (word(ast, i + 1, "=") || word(ast, i + 1, ";") || word(ast, i + 1, ",") || word(ast, i + 1, ")") ||
                                          word(ast, i + 1, ":"))) {   /* the last, a `parallel` binder */
            o->by_pointer = false; found = true;
        }
    }
    return found && !(o->value_copy && o->by_pointer);
}

/* `( T b : r )` over a `range` variable `r` by value, the detail `b : r (range)`:
 * the one-binder form (§5.7, rule 6). False for every other header. */
static bool analyze_range_var(const KAst *ast, const KIsland *is, size_t kw, const KAstNode *fn, RangeVar *r) {
    if (!word(ast, kw, "foreach") || !word(ast, kw + 1, "(")) return false;
    r->close = kw + 6;
    if (!word(ast, r->close, ")") || !word(ast, kw + 4, ":")) return false;
    if (!word_like(ast, kw + 2) || !word_like(ast, kw + 3) || !word_like(ast, kw + 5)) return false;
    r->type = kw + 2; r->binder = kw + 3; r->colon = kw + 4; r->cont = kw + 5;
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len;
    if (len < 7 || memcmp(d + len - 7, "(range)", 7) != 0) return false;
    /* the range by value (`range r`) or by pointer (`range *r`, its declaration): the temporaries
     * read `r` or `*r` (§5.7, rule 6); any other use of the name before the loop is refused */
    KToken c = k_em_tok(ast, r->cont);
    bool found = false;
    r->by_pointer = false;
    for (size_t i = fn->first + 1; i < kw && !found; i++) {
        KToken t = k_em_tok(ast, i);
        if (t.len != c.len || memcmp(t.ptr, c.ptr, c.len) != 0) continue;
        if (word(ast, i - 1, "*")) {
            if (!word(ast, i - 2, "range")) return false;
            r->by_pointer = true;
            found = true;
            continue;
        }
        if (word_like(ast, i - 1) && (word(ast, i + 1, "=") || word(ast, i + 1, ";") || word(ast, i + 1, ",") || word(ast, i + 1, ")")))
            found = true;
    }
    if (!found) return false;
    return read_body(ast, r->close, &r->body, &r->last, &r->braced);
}

/* `foreach (T b : c)` over a variable `c` of a type other than `range` that declares `first` and
 * `limit` (spec §5.1, Countable): the one-binder form (§5.7, rule 6), the bounds read by the type's
 * own verbs. The detail is `b : c (<type>)`; the type's symbol comes from its type island. False for
 * every other header, and for `range`, which analyze_range_var takes. */
typedef struct {
    size_t type, binder, colon, cont;
    size_t close, body, last;
    bool braced;
    bool by_pointer;            /* `T *c`: the verbs take `c`; by value they take `&c` */
    const char *sym; size_t sym_len;   /* the container's type symbol */
} Countable;

static bool analyze_countable_var(const KAst *ast, const KIsland *is, size_t kw, const KAstNode *fn, Countable *c) {
    if (!word(ast, kw, "foreach") || !word(ast, kw + 1, "(")) return false;
    c->close = kw + 6;
    if (!word(ast, c->close, ")") || !word(ast, kw + 4, ":")) return false;
    if (!word_like(ast, kw + 2) || !word_like(ast, kw + 3) || !word_like(ast, kw + 5)) return false;
    c->type = kw + 2; c->binder = kw + 3; c->colon = kw + 4; c->cont = kw + 5;
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, open = len;
    while (open > 0 && d[open - 1] != '(') open--;
    if (open == 0 || len < open + 2 || d[len - 1] != ')') return false;
    size_t tl = len - 1 - open;
    if (tl == 5 && memcmp(d + open, "range", 5) == 0) return false;   /* analyze_range_var's */
    if (!k_em_type_symbol(ast, d + open, tl, &c->sym, &c->sym_len)) return false;
    /* by pointer (`T *c` in the function) or by value (`T c`): the same declaration scan as analyze_over */
    KToken t = k_em_tok(ast, c->cont);
    bool found = false;
    for (size_t i = fn->first + 1; i < kw && !found; i++) {
        KToken x = k_em_tok(ast, i);
        if (x.len != t.len || memcmp(x.ptr, t.ptr, t.len) != 0) continue;
        if (word(ast, i - 1, "*")) { c->by_pointer = true; found = true; }
        else if (word_like(ast, i - 1) && (word(ast, i + 1, "=") || word(ast, i + 1, ";") || word(ast, i + 1, ",") || word(ast, i + 1, ")") ||
                                          word(ast, i + 1, ":"))) {
            c->by_pointer = false; found = true;
        }
    }
    if (!found) return false;
    return read_body(ast, c->close, &c->body, &c->last, &c->braced);
}

/* the island of the `foreach` at token `kw`, or NULL */
static const KIsland *island_at(const KAst *ast, size_t kw) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if ((is->kind == K_ISLAND_FOREACH || is->kind == K_ISLAND_WALK) && is->anchor == kw) return is;
    }
    return NULL;
}

/* appends x when its position is in the line [a, b) */
static void push_in_line(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, Edit x) {
    if (x.at >= a && x.at < b) k_em_push_edit(e, ed, cnt, x);
}

static void put_uint(Emit *e, size_t v) {
    char dig[24];
    size_t n = 0;
    do { dig[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) k_em_put_bytes(e, &dig[--n], 1);
}

static void put_tok(Emit *e, size_t i) { KToken t = k_em_tok(e->ast, i); k_em_put_bytes(e, t.ptr, t.len); }
static void put_s(Emit *e, const char *s) { k_em_put_bytes(e, s, strlen(s)); }

/* the text in e->buf, moved to the scratch of this line */
static const char *take(Emit *e, size_t *len) {
    char *s = k_em_scratch(e, e->n);
    if (!s) { e->n = 0; return NULL; }
    memcpy(s, e->buf, e->n);
    *len = e->n;
    e->n = 0;
    return s;
}

/* the binder's declaration over an `array`: `T v = arr[i];`, or `T *v = &arr[i];` (the scratch) */
static const char *array_value(Emit *e, const Over *o, size_t *vl) {
    const KAst *ast = e->ast;
    e->n = 0;
    k_em_put_range(e, o->vtype_a, o->vtype_b);
    if (!word(ast, o->vtype_b - 1, "*")) put_s(e, " ");
    put_tok(e, o->value);
    put_s(e, o->ptr_binder ? " = &" : " = ");
    k_em_put_range(e, o->cont, o->cont + 1);
    put_s(e, "["); put_tok(e, o->index); put_s(e, "];");
    return take(e, vl);
}

/* the binder's declaration over a container: `T v = S_get(keel__c<N>, i);` (the scratch) */
static const char *container_value(Emit *e, size_t n, const Over *o, size_t *vl) {
    const KAst *ast = e->ast;
    e->n = 0;
    k_em_put_range(e, o->vtype_a, o->vtype_b);
    if (!word(ast, o->vtype_b - 1, "*")) k_em_put_bytes(e, " ", 1);
    put_tok(e, o->value);
    k_em_put_bytes(e, " = ", 3);
    k_em_put_bytes(e, o->sym, o->sym_len);
    if (o->ptr_binder) {   /* the element's address (rule 3): `ptr(c, i)`, suffixed `1` when overloaded */
        k_em_put_bytes(e, "_ptr", 4);
        if (o->ptr_overloaded) k_em_put_bytes(e, "1", 1);
        k_em_put_bytes(e, "(keel__c", 8);
    } else k_em_put_bytes(e, "_get(keel__c", 12);
    put_uint(e, n);
    k_em_put_bytes(e, ", ", 2); put_tok(e, o->index);
    k_em_put_bytes(e, ");", 2);
    return take(e, vl);
}

/* Allman (backend §5.7, rule 5): the body's `{` on a line of its own, in the line [a, b), takes
 * the binder's declaration after it; the header is on another line (its `foreach` writes the block) */
static void body_brace_line(Emit *e, size_t n, const Over *o, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    if (!o->braced) return;
    size_t bo = k_em_tok_off(ast, o->body);
    if (bo < a || bo >= b) return;
    size_t vl;
    const char *val = o->array ? array_value(e, o, &vl) : container_value(e, n, o, &vl);
    if (!val) return;
    size_t ob = k_em_tok_end(ast, o->body);
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = " ", .text_len = 1 });
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = val, .text_len = vl });
}

/* `foreach (T v, I i : arr) body` over an `array` variable (spec §4.2: C syntax, no verbs):
 *   { for (I i = 0; i < N; i++) { T v = arr[i]; body } }       (T *v = &arr[i] for a pointer binder)
 * N is the dimension as the declaration writes it, or `keel__<binder>` for a parameter. */
static void edits_over_array(Emit *e, size_t n, const Over *o, size_t a, size_t b, Edit *ed, size_t *cnt) {
    (void)n;
    const KAst *ast = e->ast;
    size_t at = k_em_tok_off(ast, o->vtype_a - 2), end = k_em_tok_end(ast, o->close);   /* from the `foreach` */
    e->n = 0;
    put_s(e, "{ for (");
    k_em_put_range(e, o->itype_a, o->itype_b);
    put_s(e, " "); put_tok(e, o->index);
    put_s(e, " = 0; "); put_tok(e, o->index);
    put_s(e, " < ");
    if (o->dim_binder) put_s(e, "keel__");
    k_em_put_bytes(e, o->dim, o->dim_len);
    put_s(e, "; "); put_tok(e, o->index);
    put_s(e, "++)");
    size_t hl;
    const char *head = take(e, &hl);
    if (!head) return;
    size_t vl;
    const char *val = array_value(e, o, &vl);
    if (!val) return;
    size_t j = 0;
    for (size_t k = 0; k < *cnt; k++)
        if (ed[k].at < at || ed[k].at >= end) ed[j++] = ed[k];
    *cnt = j;
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = end - at, .text = head, .text_len = hl });
    if (o->braced) {
        size_t ob = k_em_tok_end(ast, o->body);
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = " ", .text_len = 1 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = val, .text_len = vl });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, o->last), .skip = 0, .text = " }", .text_len = 2 });
    } else {
        size_t sb = k_em_tok_off(ast, o->body);
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = "{ ", .text_len = 2 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = val, .text_len = vl });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = " ", .text_len = 1 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, o->last), .skip = 0, .text = " } }", .text_len = 4 });
    }
}

/* `foreach (T v, I i : c) body` (§5.7):
 *   { S *keel__c<N> = &c; size_t keel__n<N> = S_length(keel__c<N>); for (I i = 0; i < keel__n<N>; i++) { T v = S_get(keel__c<N>, i); body } }
 * The header becomes the text up to the value binder's line; a simple body gets
 * the braces around it. All of it is on the line of the `foreach` (rule 5). */
static void edits_over(Emit *e, const KAstNode *fn, size_t kw, const Over *o, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t n = k_em_capture_ordinal(ast, fn, kw);
    size_t at = k_em_tok_off(ast, kw), end = k_em_tok_end(ast, o->close);
    if (at < a || at >= b) {            /* not the header's line: the block's `}` may close the outer block here */
        if (o->braced) push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, o->last), .skip = 0, .text = " }", .text_len = 2 });
        body_brace_line(e, n, o, a, b, ed, cnt);
        return;
    }
    if (k_em_declared_at(e, o->value) || k_em_declared_at(e, o->index)) { e->ok = false; return; }
    if (o->array) { edits_over_array(e, n, o, a, b, ed, cnt); return; }
    e->n = 0;
    k_em_put_bytes(e, "{ ", 2);
    k_em_put_bytes(e, o->sym, o->sym_len);
    if (o->value_copy) k_em_put_bytes(e, " keel__c", 8);
    else k_em_put_bytes(e, " *keel__c", 9);
    put_uint(e, n);
    k_em_put_bytes(e, " = ", 3);
    if (!o->by_pointer && !o->value_copy) k_em_put_bytes(e, "&", 1);
    k_em_put_range(e, o->cont, o->cont_end);
    k_em_put_bytes(e, "; size_t keel__n", 16); put_uint(e, n);
    k_em_put_bytes(e, " = ", 3);
    k_em_put_bytes(e, o->sym, o->sym_len);
    k_em_put_bytes(e, "_length(keel__c", 15); put_uint(e, n);
    k_em_put_bytes(e, "); for (", 8);
    k_em_put_range(e, o->itype_a, o->itype_b);
    k_em_put_bytes(e, " ", 1); put_tok(e, o->index);
    k_em_put_bytes(e, " = 0; ", 6); put_tok(e, o->index);
    k_em_put_bytes(e, " < keel__n", 10); put_uint(e, n);
    k_em_put_bytes(e, "; ", 2); put_tok(e, o->index);
    k_em_put_bytes(e, "++)", 3);
    size_t hl;
    const char *head = take(e, &hl);
    if (!head) return;
    size_t vl;
    const char *val = container_value(e, n, o, &vl);   /* the value binder: `T v = S_get(keel__c<N>, i);` */
    if (!val) return;
    /* the header's tokens were renamed by the module's rules: they are all replaced */
    size_t j = 0;
    for (size_t k = 0; k < *cnt; k++)
        if (ed[k].at < at || ed[k].at >= end) ed[j++] = ed[k];
    *cnt = j;
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = end - at, .text = head, .text_len = hl });
    if (o->braced) {
        /* `{` -> `{ T v = ...;`, and the block's `}` closes the outer block too */
        size_t ob = k_em_tok_end(ast, o->body);
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = " ", .text_len = 1 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = val, .text_len = vl });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, o->last), .skip = 0, .text = " }", .text_len = 2 });
    } else {
        size_t sb = k_em_tok_off(ast, o->body);
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = "{ ", .text_len = 2 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = val, .text_len = vl });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = " ", .text_len = 1 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, o->last), .skip = 0, .text = " } }", .text_len = 4 });
    }
}

/* the symbol of a verb of the container's protocol: `<instance>_<verb>` for an instance of the
 * closure, else the module's own function of that name (backend §5.11); false when neither */
static bool verb_symbol(Emit *e, const char *sym, size_t sym_len, const char *verb, char *out, size_t cap, size_t *len) {
    const KAst *ast = e->ast;
    size_t vl = strlen(verb);
    for (size_t c = 0; c < ast->closure.len; c++) {
        const KClosure *k = keel_buffer_KClosure_ptr(&ast->closure, c);
        if (k->kind != K_CLOSURE_INSTANCE || k->symbol_len != sym_len || memcmp(k->symbol, sym, sym_len) != 0) continue;
        if (sym_len + 1 + vl > cap) return false;
        memcpy(out, sym, sym_len); out[sym_len] = '_'; memcpy(out + sym_len + 1, verb, vl);
        *len = sym_len + 1 + vl;
        return true;
    }
    for (size_t n = 0; n < ast->nodes.len; n++) {
        const KAstNode *d = keel_buffer_KAstNode_ptr(&ast->nodes, n);
        if (d->kind != K_AST_FUNCTION || d->name_first >= d->name_end) continue;
        KToken t = k_em_tok(ast, d->name_first);
        if (t.len != vl || memcmp(t.ptr, verb, vl) != 0) continue;
        size_t q = k_mangle_decl(e->module, (keel_slice_char){ vl, (char *)verb }, false, (keel_slice_char){ cap, out });
        if (q > cap) return false;
        *len = q;
        return true;
    }
    return false;
}

/* whether the tokens [a, b) spell the module's dotted name (`app.sf`: idents and dots, alternating) */
static bool names_module(const Emit *e, size_t a, size_t b) {
    size_t pos = 0;
    if (b <= a) return false;
    for (size_t i = a; i < b; i++) {
        if ((i - a) % 2 == 1) {
            if (!word(e->ast, i, ".") || pos >= e->module.len || e->module.ptr[pos] != '.') return false;
            pos++;
            continue;
        }
        KToken t = k_em_tok(e->ast, i);
        if (pos + t.len > e->module.len || memcmp(e->module.ptr + pos, t.ptr, t.len) != 0) return false;
        pos += t.len;
    }
    return pos == e->module.len;
}

/* the binder's declaration of a walk: `T *p = S_next(keel__c<N>, &c);` (the scratch); `next` is the
 * symbol of the container's `next` verb */
static const char *walk_value(Emit *e, size_t n, const Over *o, const char *next, size_t nl, size_t *vl) {
    const KAst *ast = e->ast;
    e->n = 0;
    k_em_put_range(e, o->vtype_a, o->vtype_b);
    if (!word(ast, o->vtype_b - 1, "*")) put_s(e, " ");
    put_tok(e, o->value);
    put_s(e, " = "); k_em_put_bytes(e, next, nl);
    put_s(e, "(keel__c"); put_uint(e, n);
    put_s(e, ", &"); put_tok(e, o->index);
    put_s(e, ");");
    return take(e, vl);
}

/* Allman (backend §5.7, rule 5), for a walk: the body's `{` on a line of its own, in the line [a, b),
 * takes the binder's declaration after it, as body_brace_line does for a foreach */
static void walk_brace_line(Emit *e, size_t n, const Over *o, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    if (!o->braced) return;
    size_t bo = k_em_tok_off(ast, o->body);
    if (bo < a || bo >= b) return;
    char nx[NAME_CAP];
    size_t nl;
    if (!verb_symbol(e, o->sym, o->sym_len, "next", nx, NAME_CAP, &nl)) return;
    size_t vl;
    const char *val = walk_value(e, n, o, nx, nl, &vl);
    if (!val) return;
    size_t ob = k_em_tok_end(ast, o->body);
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = " ", .text_len = 1 });
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = val, .text_len = vl });
}

/* `walk (T *p, C c : x) body` (§5.11):
 *   {   S *keel__c<N> = x; C c = S_begin(keel__c<N>); while (S_has_next(keel__c<N>, &c)) { T *p = S_next(keel__c<N>, &c); body } }
 * on the line of the `walk`. The cursor is written with its type; the verbs are the container's. */
static void edits_walk(Emit *e, const KAstNode *fn, size_t kw, const Over *o, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t n = k_em_capture_ordinal(ast, fn, kw);
    size_t at = k_em_tok_off(ast, kw), end = k_em_tok_end(ast, o->close);
    if (at < a || at >= b) {            /* not the header's line: the block's `}` may close the outer block here */
        if (o->braced) push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, o->last), .skip = 0, .text = " }", .text_len = 2 });
        walk_brace_line(e, n, o, a, b, ed, cnt);
        return;
    }
    if (k_em_declared_at(e, o->value) || k_em_declared_at(e, o->index)) { e->ok = false; return; }
    char vb[3][NAME_CAP];
    size_t vl3[3];
    if (!verb_symbol(e, o->sym, o->sym_len, "begin", vb[0], NAME_CAP, &vl3[0]) ||
        !verb_symbol(e, o->sym, o->sym_len, "has_next", vb[1], NAME_CAP, &vl3[1]) ||
        !verb_symbol(e, o->sym, o->sym_len, "next", vb[2], NAME_CAP, &vl3[2])) { e->ok = false; return; }
    e->n = 0;
    put_s(e, "{   ");
    k_em_put_bytes(e, o->sym, o->sym_len);
    /* a container that is a value (`slice`) is copied, and its verbs take the copy */
    put_s(e, o->value_copy ? " keel__c" : " *keel__c"); put_uint(e, n);
    put_s(e, " = ");
    if (!o->by_pointer && !o->value_copy) put_s(e, "&");
    k_em_put_range(e, o->cont, o->cont + 1);
    put_s(e, "; ");
    /* `lst.cursor` and `app.sf.cursor` name the module's own type, as one token or as several: only
     * the name is written, mangled (the qualifier is the module's dotted name, e->module) */
    KToken nm_tok = {0};
    bool own = false;
    size_t ncnt = o->itype_b - o->itype_a;
    if (ncnt >= 3 && ncnt % 2 == 1 && names_module(e, o->itype_a, o->itype_b - 2)) {
        own = true;
        nm_tok = k_em_tok(ast, o->itype_b - 1);
    } else if (ncnt == 1) {
        KToken ct = k_em_tok(ast, o->itype_a);
        size_t dot = ct.len;
        for (size_t q = 0; q < ct.len; q++) if (ct.ptr[q] == '.') dot = q;
        if (dot < ct.len) {
            KToken qual = { .ptr = ct.ptr, .len = dot };
            own = qual.len == e->module.len && memcmp(qual.ptr, e->module.ptr, qual.len) == 0;
            nm_tok = (KToken){ .ptr = ct.ptr + dot + 1, .len = ct.len - dot - 1 };
        }
    }
    if (own) {
        char nm[NAME_CAP];
        size_t q = k_mangle_decl(e->module, (keel_slice_char){ nm_tok.len, (char *)nm_tok.ptr }, true, (keel_slice_char){ sizeof nm, nm });
        if (q > sizeof nm) { e->ok = false; return; }
        k_em_put_bytes(e, nm, q);
    } else {
        k_em_put_range(e, o->itype_a, o->itype_b);
    }
    put_s(e, " "); put_tok(e, o->index);
    put_s(e, " = "); k_em_put_bytes(e, vb[0], vl3[0]);
    put_s(e, "(keel__c"); put_uint(e, n);
    put_s(e, "); while ("); k_em_put_bytes(e, vb[1], vl3[1]);
    put_s(e, "(keel__c"); put_uint(e, n);
    put_s(e, ", &"); put_tok(e, o->index);
    put_s(e, "))");
    size_t hl;
    const char *head = take(e, &hl);
    if (!head) return;
    size_t vl;
    const char *val = walk_value(e, n, o, vb[2], vl3[2], &vl);
    if (!val) return;
    size_t j = 0;
    for (size_t k = 0; k < *cnt; k++)
        if (ed[k].at < at || ed[k].at >= end) ed[j++] = ed[k];
    *cnt = j;
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = end - at, .text = head, .text_len = hl });
    if (o->braced) {
        size_t ob = k_em_tok_end(ast, o->body);
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = " ", .text_len = 1 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = ob, .skip = 0, .text = val, .text_len = vl });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, o->last), .skip = 0, .text = " }", .text_len = 2 });
    } else {
        size_t sb = k_em_tok_off(ast, o->body);
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = "{ ", .text_len = 2 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = val, .text_len = vl });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = sb, .skip = 0, .text = " ", .text_len = 1 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, o->last), .skip = 0, .text = " } }", .text_len = 4 });
    }
}

/* `foreach (T b : r) body` over a range variable (§5.7, rule 6):
 *   { size_t keel__f<N> = keel_range_first(r); size_t keel__l<N> = keel_range_limit(r); for (T b = keel__f<N>; b < keel__l<N>; b++) body }
 * The header is on the line of the `foreach` (rule 5). */
static void edits_range_var(Emit *e, const KAstNode *fn, size_t kw, const RangeVar *r, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t n = k_em_capture_ordinal(ast, fn, kw);
    size_t at = k_em_tok_off(ast, kw), end = k_em_tok_end(ast, r->close);
    if (at < a || at >= b) {            /* not the header's line: the block's `}` may close the `for` here */
        if (r->braced) push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, r->last), .skip = 0, .text = " }", .text_len = 2 });
        return;
    }
    if (k_em_declared_at(e, r->binder)) { e->ok = false; return; }
    /* `foreach` -> the two temporaries, then `for` */
    e->n = 0;
    put_s(e, "{ size_t keel__f"); put_uint(e, n);
    put_s(e, " = keel_range_first("); if (r->by_pointer) put_s(e, "*"); k_em_put_range(e, r->cont, r->cont + 1);
    put_s(e, "); size_t keel__l"); put_uint(e, n);
    put_s(e, " = keel_range_limit("); if (r->by_pointer) put_s(e, "*"); k_em_put_range(e, r->cont, r->cont + 1);
    put_s(e, "); for");
    size_t hl;
    const char *head = take(e, &hl);
    if (!head) return;
    /* `: r` -> `= keel__f<N>; b < keel__l<N>` */
    put_s(e, "= keel__f"); put_uint(e, n);
    put_s(e, "; "); put_tok(e, r->binder);
    put_s(e, " < keel__l"); put_uint(e, n);
    size_t ql;
    const char *range_text = take(e, &ql);
    if (!range_text) return;
    /* `)` -> `; b++)` */
    put_s(e, "; "); put_tok(e, r->binder); put_s(e, "++");
    size_t cl;
    const char *close_text = take(e, &cl);
    if (!close_text) return;

    /* the header's tokens were renamed by the module's rules: they are all replaced */
    size_t j = 0;
    for (size_t k = 0; k < *cnt; k++)
        if (ed[k].at < at || ed[k].at >= end) ed[j++] = ed[k];
    *cnt = j;
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok(ast, kw).len, .text = head, .text_len = hl });
    /* auto -> size_t; a written type stays */
    if (word(ast, r->type, "auto"))
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, r->type),
            .skip = k_em_tok(ast, r->type).len, .text = "size_t", .text_len = 6 });
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, r->colon),
        .skip = k_em_tok_end(ast, r->cont) - k_em_tok_off(ast, r->colon), .text = range_text, .text_len = ql });
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, r->close),
        .skip = 0, .text = close_text, .text_len = cl });
    if (r->braced) {
        /* the block's `}` closes the `for` as well */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, r->last),
            .skip = 0, .text = " }", .text_len = 2 });
    } else {
        /* one statement: a block around it, closed after its `;` */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, r->body),
            .skip = 0, .text = "{ ", .text_len = 2 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, r->last),
            .skip = 0, .text = " } }", .text_len = 4 });
    }
}

/* `foreach (T b : c) body` over a Countable variable (§5.7, rule 6):
 *   { size_t keel__f<N> = S_first(c); size_t keel__l<N> = S_limit(c); for (T b = keel__f<N>; b < keel__l<N>; b++) body }
 * `c` is passed as `&c` by value, as `c` when it is a pointer. The header is on the line of the
 * `foreach` (rule 5). */
static void edits_countable_var(Emit *e, const KAstNode *fn, size_t kw, const Countable *c, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t n = k_em_capture_ordinal(ast, fn, kw);
    size_t at = k_em_tok_off(ast, kw), end = k_em_tok_end(ast, c->close);
    if (at < a || at >= b) {            /* not the header's line: the block's `}` may close the `for` here */
        if (c->braced) push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, c->last), .skip = 0, .text = " }", .text_len = 2 });
        return;
    }
    if (k_em_declared_at(e, c->binder)) { e->ok = false; return; }
    char vf[NAME_CAP], vl_[NAME_CAP];
    size_t lf, ll;
    if (!verb_symbol(e, c->sym, c->sym_len, "first", vf, NAME_CAP, &lf) ||
        !verb_symbol(e, c->sym, c->sym_len, "limit", vl_, NAME_CAP, &ll)) { e->ok = false; return; }
    /* the bounds: `first` and `limit` of the container, by pointer or of its address */
    e->n = 0;
    put_s(e, "{ size_t keel__f"); put_uint(e, n);
    put_s(e, " = "); k_em_put_bytes(e, vf, lf);
    put_s(e, "("); if (!c->by_pointer) put_s(e, "&"); k_em_put_range(e, c->cont, c->cont + 1);
    put_s(e, "); size_t keel__l"); put_uint(e, n);
    put_s(e, " = "); k_em_put_bytes(e, vl_, ll);
    put_s(e, "("); if (!c->by_pointer) put_s(e, "&"); k_em_put_range(e, c->cont, c->cont + 1);
    put_s(e, "); for");
    size_t hl;
    const char *head = take(e, &hl);
    if (!head) return;
    /* `: c` -> `= keel__f<N>; b < keel__l<N>` */
    put_s(e, "= keel__f"); put_uint(e, n);
    put_s(e, "; "); put_tok(e, c->binder);
    put_s(e, " < keel__l"); put_uint(e, n);
    size_t ql;
    const char *range_text = take(e, &ql);
    if (!range_text) return;
    /* `)` -> `; b++)` */
    put_s(e, "; "); put_tok(e, c->binder); put_s(e, "++");
    size_t cl;
    const char *close_text = take(e, &cl);
    if (!close_text) return;

    /* the header's tokens are all replaced, as in edits_range_var */
    size_t j = 0;
    for (size_t k = 0; k < *cnt; k++)
        if (ed[k].at < at || ed[k].at >= end) ed[j++] = ed[k];
    *cnt = j;
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok(ast, kw).len, .text = head, .text_len = hl });
    /* auto -> size_t; a written type stays */
    if (word(ast, c->type, "auto"))
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, c->type),
            .skip = k_em_tok(ast, c->type).len, .text = "size_t", .text_len = 6 });
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, c->colon),
        .skip = k_em_tok_end(ast, c->cont) - k_em_tok_off(ast, c->colon), .text = range_text, .text_len = ql });
    push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, c->close),
        .skip = 0, .text = close_text, .text_len = cl });
    if (c->braced) {
        /* the block's `}` closes the `for` as well */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, c->last),
            .skip = 0, .text = " }", .text_len = 2 });
    } else {
        /* one statement: a block around it, closed after its `;` */
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, c->body),
            .skip = 0, .text = "{ ", .text_len = 2 });
        push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_end(ast, c->last),
            .skip = 0, .text = " } }", .text_len = 4 });
    }
}

static const KAstNode *fn_of(const KAst *ast, size_t tok) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind == K_AST_FUNCTION && n->body_first != NO_TOKEN && n->body_end != NO_TOKEN &&
            tok >= n->body_first && tok < n->body_end) return n;
    }
    return NULL;
}

void k_em_edits_foreach(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t kw = fn->body_first; kw < fn->body_end; kw++) {
        if (!word(ast, kw, "foreach") && !word(ast, kw, "walk")) continue;
        const KIsland *is = island_at(ast, kw);
        Over o;
        if (is && analyze_over(ast, is, kw, fn, &o)) {
            if (o.walk) edits_walk(e, fn, kw, &o, a, b, ed, cnt);
            else edits_over(e, fn, kw, &o, a, b, ed, cnt);
            continue;
        }
        RangeVar rv;
        if (is && analyze_range_var(ast, is, kw, fn, &rv)) { edits_range_var(e, fn, kw, &rv, a, b, ed, cnt); continue; }
        Countable cv;
        if (is && analyze_countable_var(ast, is, kw, fn, &cv)) { edits_countable_var(e, fn, kw, &cv, a, b, ed, cnt); continue; }
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
    if (is->kind != K_ISLAND_FOREACH && is->kind != K_ISLAND_WALK) return false;
    /* the detail of a range is `<binder> : <A>..<B>`; over a container it has no `..` */
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    bool dots = false;
    for (size_t i = 0; i + 1 < is->text_len; i++)
        if (d[i] == '.' && d[i + 1] == '.') { dots = true; break; }
    if (!dots) {
        const KAstNode *fn = fn_of(ast, is->anchor);
        Over o;
        RangeVar rv;
        Countable cv;
        return fn && (analyze_over(ast, is, is->anchor, fn, &o) ||
                      analyze_range_var(ast, is, is->anchor, fn, &rv) ||
                      analyze_countable_var(ast, is, is->anchor, fn, &cv));
    }
    Foreach f;
    return analyze(ast, is->anchor, &f);
}
