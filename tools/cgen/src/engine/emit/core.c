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
 *   keel.dim(v, 0)              ->   keel__n
 *
 * The argument of `keel.length` over a module array keeps its bytes: its rename
 * is the island the parser left on it. The other forms are written whole, and
 * an edit another construction made inside them is dropped (emit.c, drop_nested).
 *
 * Refused (k_em_handles_core answers false, and the module is refused with
 * implementation-limit): a binder with `keel.capacity`; a parameter without a
 * binder (its extent is not in the C type, `i32 v[static 4]`); `keel.length`
 * over a module array whose dimensions are not literal; a `keel.dim` index
 * that is not a decimal literal, or not below the number of dimensions; and an
 * element type that names a declaration of the module, which would have to be
 * renamed inside the generated text. */
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

/* whether the module declares a name any token of [a, b) spells */
static bool names_declaration(const KAst *ast, size_t a, size_t b) {
    for (size_t i = a; i < b; i++) {
        if (!ident(ast, i)) continue;
        KToken t = k_em_tok(ast, i);
        for (size_t k = 0; k < ast->nodes.len; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            if (n->name_first >= n->name_end) continue;
            KToken nm = k_em_tok(ast, n->name_first);
            if (nm.len == t.len && memcmp(nm.ptr, t.ptr, t.len) == 0) return true;
        }
    }
    return false;
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
    if (strcmp(c->verb, "keel.dim") == 0) {
        long k;
        if (!tok_is(ast, i + 5, ",") || !decimal_at(ast, i + 6, &k) || !tok_is(ast, i + 7, ")")) return false;
        c->index = i + 6;
        c->close = i + 7;
    } else {
        if (!tok_is(ast, i + 5, ")")) return false;
        c->index = NO_TOKEN;
        c->close = i + 5;
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
    if (param && !r.binder) return false;       /* no extent in the C type of `i32 v[static 4]` */
    if (r.binder) {
        /* backend §5.18: dimension 0 is keel__<name>, the extent that arrived */
        if (capacity) return false;
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
        /* a module array's extent is sizeof only when every dimension is a literal */
        for (size_t k = 0; k < r.ndims; k++) {
            long v;
            if (r.dim_end[k] != r.dim_first[k] + 1 || !decimal_at(ast, r.dim_first[k], &v)) return false;
        }
    }
    p->type_a = r.arr + 1;
    p->type_b = r.type_b;
    if (p->type_b <= p->type_a || names_declaration(ast, p->type_a, p->type_b)) return false;
    return true;
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
        /* `)` -> `)/sizeof(<element type>)` */
        size_t ta = k_em_tok_off(ast, p.type_a), tb = k_em_tok_end(ast, p.type_b - 1);
        at = k_em_tok_off(ast, c.close);
        if (at >= a && at < b) {
            char *w = k_em_scratch(e, (tb - ta) + 11);
            if (!w) return;
            size_t q = 0;
            memcpy(w + q, ")/sizeof(", 9); q += 9;
            memcpy(w + q, ast->source.ptr + ta, tb - ta); q += tb - ta;
            w[q++] = ')';
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 1, .text = w, .text_len = q });
        }
    }
}
