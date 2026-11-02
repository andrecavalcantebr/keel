/* engine/emit/core.c — the verbs of the core (emit_internal.h; codegen design
 * E5: one file per construction; backend §5.3, rule 7; §5.18, rule 5; spec §4.2).
 *
 * The verbs over an `array` are the core's, not calls to a module, and come out
 * as constant expressions, so they never reach a function. The element type
 * comes from the declaration, not from `*(v)` (rule 7), so a column of pointers
 * keeps its own type.
 *
 *   pub array i32 v[6] = { … };
 *   keel.length(v)              ->   sizeof(app_ofr_v)/sizeof(i32)
 *   array i32 x[2, 3];          (a local)
 *   keel.length(x)              ->   sizeof(x)/sizeof(i32)
 *   keel.capacity(x)            ->   sizeof(x)/sizeof(i32)
 *   keel.dim(x, 1)              ->   3
 *   keel.dim(g, 0)              ->   4         (g: pub array i32 g[4])
 *
 * A parameter with a dimension binder (`array i32 v[size_t n]`, §5.18) has no
 * extent of its own: its dimension 0 is the `keel__n` that arrived, so
 *
 *   keel.length(v)              ->   keel__n   (times the other dimensions)
 *   keel.capacity(v)            ->   keel__n
 *   keel.dim(v, 0)              ->   keel__n
 *
 * A parameter without a binder (`array i32 v[static 2][3]`, backend §5.3 rule 7)
 * has no extent in its C type, so its length is the product of its literal
 * dimensions:
 *
 *   keel.length(v)              ->   2 * 3
 *   keel.dim(v, 1)              ->   3
 *
 * A field of a struct (`b->v`, `b.v`, the container named by its own name) is
 * a declaration of the struct, read as the others are; its dimensions may be a
 * constexpr of a decimal literal, like a module array's (`priv array i32 tab[K]`):
 *
 *   keel.length(b->v)           ->   sizeof(b->v)/sizeof(i32)
 *   keel.length(tab)            ->   sizeof(app_lf_tab)/sizeof(i32)     (K = 3)
 *
 * The element type of the `sizeof` is written as the C text names it, a module
 * declaration renamed (`array Box bs[3]`: sizeof(bs)/sizeof(app_al_Box)).
 * The argument of `keel.length` over a module array keeps its bytes: its rename
 * is the island the parser left on it. The other forms are written whole, and
 * an edit another construction made inside them is dropped (emit.c, drop_nested).
 *
 * Refused (k_em_handles_core answers false, and the module is refused with
 * implementation-limit): `keel.capacity` over a parameter without a binder;
 * `keel.length` over a module array whose dimensions are not decimal (a literal,
 * or a constexpr of one); a `keel.dim` index that is not a decimal literal, or not
 * below the number of dimensions. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define ARROW "\xE2\x86\x92"
#define NO_TOKEN ((size_t)-1)
#define TEXT_CAP 128            /* the whole text a verb is written as: the binder's product */

static bool tok_is(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

static bool ident(const KAst *ast, size_t i) {
    return i < ast->tokens.len && k_token_is_ident(k_em_tok(ast, i));
}

/* whether the island's detail is `<verb> → core …` */
static bool is_core_verb(const KAst *ast, const KIsland *is, const char *verb) {
    if (is->kind != K_ISLAND_CALL) return false;
    const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, arrow = len;
    for (size_t k = 0; k + 3 <= len; k++)
        if (memcmp(d + k, ARROW, 3) == 0) { arrow = k; break; }
    if (arrow == len) return false;
    size_t s = arrow + 3;
    while (s < len && d[s] == ' ') s++;
    if (len - s < 4 || memcmp(d + s, "core", 4) != 0) return false;
    size_t v = strlen(verb);
    return arrow > v && memcmp(d, verb, v) == 0 && d[v] == '/';
}

/* the decimal literal at token i, its value in *v */
static bool decimal_at(const KAst *ast, size_t i, long *v) {
    if (i >= ast->tokens.len) return false;
    KToken t = k_em_tok(ast, i);
    if (t.len == 0 || t.len > 9) return false;
    long n = 0;
    for (size_t k = 0; k < t.len; k++) {
        if (t.ptr[k] < '0' || t.ptr[k] > '9') return false;
        n = n * 10 + (t.ptr[k] - '0');
    }
    *v = n;
    return true;
}

/* the function whose tokens hold token tok, or NULL */
static const KAstNode *enclosing_fn(const KAst *ast, size_t tok) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind == K_AST_FUNCTION && n->first <= tok && tok < n->end) return n;
    }
    return NULL;
}

/* One call of a verb: `keel.length(x)`, `keel.capacity(x)`, `keel.dim(x, k)`. */
typedef struct {
    const char *verb;           /* the island's verb: "keel.length", "keel.capacity", "keel.dim" */
    size_t open;                /* the `(` */
    size_t arg;                 /* the name of the array */
    size_t index;               /* keel.dim: the decimal index; NO_TOKEN for the others */
    size_t close;               /* the `)` */
} Call;

/* The call the island is, when it is one of the verbs over an array. */
static bool read_call(const KAst *ast, const KIsland *is, Call *c) {
    static const char *const verbs[] = { "keel.length", "keel.capacity", "keel.dim" };
    c->verb = NULL;
    for (size_t k = 0; k < sizeof verbs / sizeof verbs[0] && !c->verb; k++)
        if (is_core_verb(ast, is, verbs[k])) c->verb = verbs[k];
    if (!c->verb) return false;
    size_t i = is->anchor;
    /* `keel . <verb after "keel."> (` */
    if (!tok_is(ast, i, "keel") || !tok_is(ast, i + 1, ".") || !tok_is(ast, i + 2, c->verb + 5)) return false;
    if (!tok_is(ast, i + 3, "(") || !ident(ast, i + 4)) return false;
    c->open = i + 3;
    c->arg = i + 4;
    /* a field, `b->v` or `b.v`: the argument is the container's name, and its field is read from its type */
    size_t q = i + 5;
    if (tok_is(ast, q, "->") || tok_is(ast, q, ".")) {
        if (!ident(ast, q + 1)) return false;
        q += 2;
    }
    if (strcmp(c->verb, "keel.dim") == 0) {
        long k;
        if (!tok_is(ast, q, ",") || !decimal_at(ast, q + 1, &k) || !tok_is(ast, q + 2, ")")) return false;
        c->index = q + 1;
        c->close = q + 2;
    } else {
        if (!tok_is(ast, q, ")")) return false;
        c->index = NO_TOKEN;
        c->close = q;
    }
    return true;
}

/* What a call becomes: either a whole text (`text`, the call replaced), or the
 * sizeof of the array with its element type (the type's tokens [type_a, type_b)). */
typedef struct {
    bool whole;
    char text[TEXT_CAP];
    size_t len;
    size_t type_a, type_b;
} Plan;

/* appends s to p->text, or fails when the text would not fit */
static bool put(Plan *p, const char *s, size_t n) {
    if (p->len + n >= TEXT_CAP) return false;
    memcpy(p->text + p->len, s, n);
    p->len += n;
    return true;
}

/* a decimal number, v >= 0 (a dimension of an array) */
static bool put_number(Plan *p, long v) {
    char b[20];
    size_t n = 0;
    do { b[sizeof b - 1 - n++] = (char)('0' + v % 10); v /= 10; } while (v > 0);
    return put(p, b + sizeof b - n, n);
}

/* Plans the call c, made inside function fn. False when the core does not lower it. */
static bool plan(const KAst *ast, const KAstNode *fn, const Call *c, Plan *p) {
    KArrayRef r;
    if (!k_em_array_of(ast, fn, c->arg, &r)) return false;
    bool module = r.arr < fn->first;
    bool param = !module && r.arr < fn->body_first;
    bool capacity = strcmp(c->verb, "keel.capacity") == 0;
    bool dim = strcmp(c->verb, "keel.dim") == 0;
    p->whole = false;
    p->len = 0;
    p->type_a = p->type_b = 0;
    if (param && !r.binder && capacity) return false;   /* not lowered yet */
    if (param && !r.binder && !dim) {
        /* backend §5.3, rule 7: the extent of a parameter without a binder is not in its C
         * type (`i32 v[static 2][3]` is a pointer), so its length is the product of the
         * literal dimensions; its dim is the literal dimension below (the common branch) */
        p->whole = true;
        for (size_t k = 0; k < r.ndims; k++) {
            long v;
            if (!k_em_array_dim(ast, fn, &r, k, &v)) return false;
            if (k > 0 && !put(p, " * ", 3)) return false;
            if (!put_number(p, v)) return false;
        }
        return true;
    }
    if (r.binder) {
        /* backend §5.18: dimension 0 is keel__<name>, the extent that arrived; a capacity is
         * the same extent as a length */
        size_t nm = r.dim_end[0] - 1;
        p->whole = true;
        if (dim) {
            long k;
            if (!decimal_at(ast, c->index, &k) || k >= (long)r.ndims) return false;
            if (k == 0) {
                if (!put(p, "keel__", 6) || !put(p, k_em_tok(ast, nm).ptr, k_em_tok(ast, nm).len)) return false;
                return true;
            }
            long v;
            return k_em_array_dim(ast, fn, &r, (size_t)k, &v) && put_number(p, v);
        }
        /* length: keel__n, times the other dimensions (each a decimal) */
        if (r.ndims > 1 && !put(p, "(", 1)) return false;
        if (!put(p, "keel__", 6) || !put(p, k_em_tok(ast, nm).ptr, k_em_tok(ast, nm).len)) return false;
        for (size_t k = 1; k < r.ndims; k++) {
            long v;
            if (!k_em_array_dim(ast, fn, &r, k, &v) || !put(p, " * ", 3) || !put_number(p, v)) return false;
        }
        return !(r.ndims > 1 && !put(p, ")", 1));
    }
    if (dim) {
        long k, v;
        if (!decimal_at(ast, c->index, &k) || k >= (long)r.ndims) return false;
        if (!k_em_array_dim(ast, fn, &r, (size_t)k, &v)) return false;
        p->whole = true;
        return put_number(p, v);
    }
    /* length and capacity: the table's own extent, sizeof over the element type */
    if (module) {
        /* a module array's extent is sizeof only when every dimension is decimal (a literal, or a constexpr of one) */
        for (size_t k = 0; k < r.ndims; k++) {
            long v;
            if (!k_em_array_dim(ast, fn, &r, k, &v)) return false;
        }
    }
    p->type_a = r.arr + 1;
    p->type_b = r.type_b;
    return p->type_b > p->type_a;
}

bool k_em_handles_core(const KAst *ast, const KIsland *is) {
    const KAstNode *fn = enclosing_fn(ast, is->anchor);
    Call c;
    Plan p;
    return fn && read_call(ast, is, &c) && plan(ast, fn, &c, &p);
}

void k_em_edits_core(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->anchor < n->first || is->anchor >= n->end) continue;
        Call c;
        Plan p;
        if (!read_call(ast, is, &c) || !plan(ast, n, &c, &p)) continue;
        size_t at = k_em_tok_off(ast, is->anchor);
        if (p.whole) {
            /* the whole call, `keel` to `)`, becomes its text */
            if (at < a || at >= b) continue;
            char *w = k_em_scratch(e, p.len);
            if (!w) return;
            memcpy(w, p.text, p.len);
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at,
                .skip = k_em_tok_end(ast, c.close) - at, .text = w, .text_len = p.len });
            continue;
        }
        /* `keel.length(` -> `sizeof(` */
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at,
                .skip = k_em_tok_end(ast, c.open) - at, .text = "sizeof(", .text_len = 7 });
        /* `)` -> `)/sizeof(<element type>)`: the type's tokens after the `)` (an insertion,
         * so the replacement of the `)` comes first), renamed as in the C text */
        at = k_em_tok_off(ast, c.close);
        if (at >= a && at < b) {
            size_t after = k_em_tok_end(ast, c.close);
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 1, .text = ")/sizeof(", .text_len = 9 });
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TOKS, .at = after, .skip = 0, .tok_a = p.type_a, .tok_b = p.type_b });
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = after, .skip = 0, .text = ")", .text_len = 1 });
        }
    }
}
