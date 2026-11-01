/* engine/emit/extent.c — `extent` (emit_internal.h; codegen design E5: one file
 * per construction; backend §5.15, spec §4.11).
 *
 * The declaration comes out as the author wrote it, minus what is keel's: the
 * word `extent`, the group brackets, and the `array` of each column (§5.15,
 * rule 1). No `typedef` is written: the program spells `struct NAME`.
 *
 *   pub extent struct grid [rows, rcap] [cols, ccap] {    struct app_pos_grid {
 *       array i32 *v;                                         i32 *v;
 *       size_t rows, cols, rcap, ccap;                         size_t rows, cols, rcap, ccap;
 *   };                                                    };
 *
 * An embedded column keeps its dimensions, each comma becoming a bracket pair
 * as in any `array` (§5.3): `array u8 r[H, W]` is `u8 r[app_pos_H][app_pos_W]`.
 *
 * One `static inline` accessor per column (§5.15, rules 2, 4 and 6) carries the
 * `#line` of the column it belongs to, and holds one `KEEL_CHECK` per index with
 * the group's whole invariant. The address is Horner over the inner capacities
 * for a column by pointer, and successive brackets for an embedded one.
 *
 * At the use point, `P.col[…]` is `*f(&P, …)` and `P->col[…]` is `*f(P, …)`
 * (rule 5), by the column island the parser leaves there. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define NO_TOKEN ((size_t)-1)
#define MAX_GROUPS 4
#define MAX_COLUMNS 8

/* one extent declaration, read from the tokens of its node */
typedef struct {
    size_t kw;                      /* the word `extent` */
    size_t name;                    /* the tag's name */
    size_t open, close;             /* the body's `{` and `}` */
    size_t groups;
    size_t count[MAX_GROUPS], cap[MAX_GROUPS];     /* the group's count and capacity names */
    size_t lbr[MAX_GROUPS], rbr[MAX_GROUPS];       /* its brackets */
    size_t columns;
    size_t arr[MAX_COLUMNS];        /* the column's `array` */
    size_t col[MAX_COLUMNS];        /* its name */
    size_t type_a[MAX_COLUMNS], type_b[MAX_COLUMNS];   /* the element type's tokens */
    size_t dlbr[MAX_COLUMNS], drbr[MAX_COLUMNS];       /* an embedded column's dimensions */
    bool embedded[MAX_COLUMNS];
    size_t fields;
    size_t field[32];               /* every field's name, for a capacity held in one */
} Extent;

static bool tok_is(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

static bool ident(const KAst *ast, size_t i) {
    return i < ast->tokens.len && k_token_is_ident(k_em_tok(ast, i));
}

static bool opens(const KAst *ast, size_t i) {
    return tok_is(ast, i, "[") || tok_is(ast, i, "(") || tok_is(ast, i, "{");
}

static bool closes(const KAst *ast, size_t i) {
    return tok_is(ast, i, "]") || tok_is(ast, i, ")") || tok_is(ast, i, "}");
}

/* the token that closes the pair `open` starts, or NO_TOKEN */
static size_t closer(const KAst *ast, size_t open, size_t limit) {
    int depth = 0;
    for (size_t i = open; i < limit; i++) {
        if (opens(ast, i)) depth++;
        else if (closes(ast, i) && --depth == 0) return i;
    }
    return NO_TOKEN;
}

/* the blanks that follow token i and leave with it */
static size_t blanks_after(const KAst *ast, size_t i) {
    size_t p = k_em_tok_end(ast, i), lim = k_em_tok_off(ast, i + 1);
    const char *s = ast->source.ptr;
    while (p < lim && (s[p] == ' ' || s[p] == '\t')) p++;
    return p - k_em_tok_end(ast, i);
}

/* Whether the node is an `extent struct` declaration, and its parts (the shape
 * the parser already accepted: spec §4.11). */
static bool k_em_extent_at(const KAst *ast, const KAstNode *n, Extent *x) {
    if (n->kind != K_AST_TYPE || n->name_first >= n->name_end) return false;
    size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
    if (!tok_is(ast, k, "extent") || !tok_is(ast, k + 1, "struct")) return false;
    x->kw = k;
    x->name = n->name_first;
    /* the groups, between the name and the body */
    x->groups = 0;
    size_t i = x->name + 1;
    while (tok_is(ast, i, "[")) {
        size_t c = closer(ast, i, n->end);
        if (c == NO_TOKEN || x->groups >= MAX_GROUPS) return false;
        if (!ident(ast, i + 1) || !tok_is(ast, i + 2, ",") || c != i + 4) return false;
        x->lbr[x->groups] = i;
        x->rbr[x->groups] = c;
        x->count[x->groups] = i + 1;
        x->cap[x->groups] = i + 3;
        x->groups++;
        i = c + 1;
    }
    if (!x->groups || !tok_is(ast, i, "{")) return false;
    x->open = i;
    x->close = closer(ast, i, n->end);
    if (x->close == NO_TOKEN) return false;
    /* the fields: each one ends at a `;`, each declarator at a `,` */
    x->columns = x->fields = 0;
    for (size_t f = x->open + 1; f < x->close; ) {
        size_t end = f;
        int depth = 0;
        while (end < x->close && !(depth == 0 && tok_is(ast, end, ";"))) {
            if (opens(ast, end)) depth++;
            else if (closes(ast, end)) depth--;
            end++;
        }
        bool column = tok_is(ast, f, "array");
        size_t type_a = column ? f + 1 : f;
        size_t name = NO_TOKEN, dims = NO_TOKEN;
        depth = 0;
        for (size_t p = f; p <= end; p++) {
            bool stop = p == end || (depth == 0 && tok_is(ast, p, ","));
            if (!stop) {
                if (tok_is(ast, p, "[") && depth == 0 && dims == NO_TOKEN) dims = p;
                if (opens(ast, p)) depth++;
                else if (closes(ast, p)) depth--;
                else if (depth == 0 && dims == NO_TOKEN && ident(ast, p)) name = p;
                continue;
            }
            if (name != NO_TOKEN) {
                if (x->fields < 32) x->field[x->fields++] = name;
                if (column) {
                    if (x->columns >= MAX_COLUMNS) return false;
                    x->arr[x->columns] = f;
                    x->col[x->columns] = name;
                    x->type_a[x->columns] = type_a;
                    x->type_b[x->columns] = name;   /* `f32 *` keeps the `*` out of the type */
                    x->embedded[x->columns] = dims != NO_TOKEN;
                    x->dlbr[x->columns] = dims;
                    x->drbr[x->columns] = dims == NO_TOKEN ? NO_TOKEN : closer(ast, dims, end + 1);
                    x->columns++;
                }
            }
            name = dims = NO_TOKEN;
        }
        f = end + 1;
    }
    return x->columns > 0;
}

bool k_em_is_extent(const KAst *ast, const KAstNode *n) {
    Extent x;
    return k_em_extent_at(ast, n, &x);
}

/* whether the extent holds a field named like token `tok` */
static bool is_field(const KAst *ast, const Extent *x, size_t tok) {
    KToken t = k_em_tok(ast, tok);
    for (size_t k = 0; k < x->fields; k++) {
        KToken f = k_em_tok(ast, x->field[k]);
        if (f.len == t.len && memcmp(f.ptr, t.ptr, t.len) == 0) return true;
    }
    return false;
}

/* The declaration's edits (rule 1): the word `extent`, the group brackets and
 * each column's `array` leave; an embedded column's commas become brackets. */
void k_em_edits_extent(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    Extent x;
    if (!k_em_extent_at(ast, n, &x)) return;
    size_t at = k_em_tok_off(ast, x.kw);
    if (at >= a && at < b)
        k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at,
            .skip = k_em_tok(ast, x.kw).len + blanks_after(ast, x.kw) });
    /* each group: from the blanks before its `[` to its `]` */
    for (size_t g = 0; g < x.groups; g++) {
        size_t from = k_em_tok_end(ast, x.lbr[g] - 1), to = k_em_tok_end(ast, x.rbr[g]);
        if (from >= a && from < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = from, .skip = to - from });
    }
    for (size_t c = 0; c < x.columns; c++) {
        at = k_em_tok_off(ast, x.arr[c]);
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = at,
                .skip = k_em_tok(ast, x.arr[c]).len + blanks_after(ast, x.arr[c]) });
        if (!x.embedded[c]) continue;
        /* `[H, W]` -> `[H][W]`: each comma of the dimensions, with its blanks */
        int depth = 0;
        for (size_t i = x.dlbr[c] + 1; i < x.drbr[c]; i++) {
            if (opens(ast, i)) { depth++; continue; }
            if (closes(ast, i)) { depth--; continue; }
            if (depth != 0 || !tok_is(ast, i, ",")) continue;
            size_t p = k_em_tok_off(ast, i);
            if (p >= a && p < b)
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = p,
                    .skip = 1 + blanks_after(ast, i), .text = "][", .text_len = 2 });
        }
        /* a dimension that names a declaration of the module takes its C name */
        for (size_t i = x.dlbr[c] + 1; i < x.drbr[c]; i++) {
            if (!ident(ast, i)) continue;
            const KAstNode *d = k_em_declared_at(e, i);
            if (!d) continue;
            size_t p = k_em_tok_off(ast, i);
            if (p >= a && p < b)
                k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_RENAME, .at = p,
                    .skip = k_em_tok(ast, i).len, .name_tok = d->name_first,
                    .is_type = d->kind == K_AST_TYPE || d->kind == K_AST_TAGS });
        }
    }
}

/* ---- the column accessors (rules 2, 4 and 6) ---- */

static void put_s(Emit *e, const char *s) { k_em_put_bytes(e, s, strlen(s)); }

static void put_index(Emit *e, size_t g) {
    char d[8];
    size_t n = 0;
    size_t v = g;
    do { d[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    put_s(e, "i");
    while (n) k_em_put_bytes(e, &d[--n], 1);
}

/* The group's capacity as C writes it: `p->ccap` for a field, the mangled name
 * for a declaration of the module, the token as written otherwise (rule 4). */
static void put_cap(Emit *e, const Extent *x, size_t g) {
    const KAst *ast = e->ast;
    size_t tok = x->cap[g];
    if (is_field(ast, x, tok)) {
        put_s(e, "p->");
        KToken t = k_em_tok(ast, tok);
        k_em_put_bytes(e, t.ptr, t.len);
        return;
    }
    const KAstNode *d = k_em_declared_at(e, tok);
    if (d) { k_em_put_mangled(e, d->name_first, d->kind == K_AST_TYPE || d->kind == K_AST_TAGS); return; }
    KToken t = k_em_tok(ast, tok);
    k_em_put_bytes(e, t.ptr, t.len);
}

/* `static inline <T> *<symbol>_<col>_ptr(struct <tag> *p, size_t i0, ...)` */
static void put_signature(Emit *e, const Extent *x, size_t c) {
    const KAst *ast = e->ast;
    put_s(e, "static inline ");
    k_em_put_range(e, x->type_a[c], x->type_b[c]);
    if (x->embedded[c]) put_s(e, " *");   /* a pointer column already writes its own `*` */
    k_em_put_mangled(e, x->name, true);
    put_s(e, "_");
    KToken col = k_em_tok(ast, x->col[c]);
    k_em_put_bytes(e, col.ptr, col.len);
    put_s(e, "_ptr(struct ");
    k_em_put_mangled(e, x->name, true);
    put_s(e, " *p");
    for (size_t g = 0; g < x->groups; g++) {
        put_s(e, ", size_t ");
        put_index(e, g);
    }
    put_s(e, ")");
}

/* the .k line a column's accessor carries: the line of its declaration */
static size_t column_line(Emit *e, const Extent *x, size_t c) {
    return k_em_line_at(e, k_em_tok_off(e->ast, x->arr[c]));
}

/* One prototype per column, each on the line of its column (§6). */
void k_em_extent_protos(Emit *e, const KAstNode *n) {
    Extent x;
    if (!k_em_extent_at(e->ast, n, &x)) return;
    for (size_t c = 0; c < x.columns && e->ok; c++) {
        e->n = 0;
        put_signature(e, &x, c);
        put_s(e, ";\n");
        if (e->ok && !k_emit_at(&e->w, column_line(e, &x, c), (keel_slice_char){ e->n, e->buf })) e->ok = false;
    }
}

/* One accessor per column: the signature, a KEEL_CHECK per index with the
 * group's whole invariant (rule 6), and the address (rule 4). */
void k_em_extent_bodies(Emit *e, const KAstNode *n) {
    const KAst *ast = e->ast;
    Extent x;
    if (!k_em_extent_at(ast, n, &x)) return;
    for (size_t c = 0; c < x.columns && e->ok; c++) {
        e->n = 0;
        put_signature(e, &x, c);
        put_s(e, " {\n");
        for (size_t g = 0; g < x.groups; g++) {
            KToken count = k_em_tok(ast, x.count[g]);
            put_s(e, "    KEEL_CHECK(");
            put_index(e, g);
            put_s(e, " < p->");
            k_em_put_bytes(e, count.ptr, count.len);
            put_s(e, " && p->");
            k_em_put_bytes(e, count.ptr, count.len);
            put_s(e, " <= ");
            put_cap(e, &x, g);
            put_s(e, ", \"extent-index-out-of-bounds\");\n");
        }
        put_s(e, "    return &p->");
        KToken col = k_em_tok(ast, x.col[c]);
        k_em_put_bytes(e, col.ptr, col.len);
        if (x.embedded[c]) {
            for (size_t g = 0; g < x.groups; g++) { put_s(e, "["); put_index(e, g); put_s(e, "]"); }
        } else {
            /* Horner over the inner capacities: ((i0 * c1 + i1) * c2 + i2) */
            put_s(e, "[");
            for (size_t g = 1; g < x.groups; g++) if (g > 1) put_s(e, "(");
            put_index(e, 0);
            for (size_t g = 1; g < x.groups; g++) {
                if (g > 1) put_s(e, ")");
                put_s(e, " * ");
                put_cap(e, &x, g);
                put_s(e, " + ");
                put_index(e, g);
            }
            put_s(e, "]");
        }
        put_s(e, ";\n}\n");
        if (e->ok && !k_emit_at(&e->w, column_line(e, &x, c), (keel_slice_char){ e->n, e->buf })) e->ok = false;
    }
}

/* ---- the use point (rule 5) ---- */

#define ARROW "\xE2\x86\x92"

/* The accessor's symbol, from the island's detail (`p->x rank 1 -> <symbol>`),
 * and the `[` of the indices. False when the island is not a column access this
 * file lowers: the path must be a chain of names (`p`, `a.b`, `s->c`), the form
 * every golden writes; an expression path, which rule 5 parenthesizes, is not
 * lowered yet. */
static bool column_at(const KAst *ast, const KIsland *is, const char **sym, size_t *sym_len,
                      size_t *open, size_t *close) {
    if (is->kind != K_ISLAND_COLUMN) return false;
    const char *t = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, arrow = len;
    for (size_t k = 0; k + 3 <= len; k++)
        if (memcmp(t + k, ARROW, 3) == 0) { arrow = k; break; }
    if (arrow == len) return false;
    size_t s = arrow + 3;
    while (s < len && t[s] == ' ') s++;
    size_t q = s;
    while (q < len && t[q] != ' ') q++;
    if (q == s) return false;
    *sym = t + s;
    *sym_len = q - s;
    /* the tokens: `<name> (('.'|'->') <name>)+ '['` from the anchor */
    size_t i = is->anchor;
    if (!ident(ast, i)) return false;
    size_t links = 0;
    for (i++; tok_is(ast, i, ".") || tok_is(ast, i, "->"); i += 2) {
        if (!ident(ast, i + 1)) return false;
        links++;
    }
    if (links == 0 || !tok_is(ast, i, "[")) return false;
    size_t c = closer(ast, i, ast->tokens.len);
    if (c == NO_TOKEN) return false;
    *open = i;
    *close = c;
    return true;
}

bool k_em_handles_column(const KAst *ast, const KIsland *is) {
    const char *sym;
    size_t sym_len, open, close;
    return column_at(ast, is, &sym, &sym_len, &open, &close);
}

/* `P->col[i, j]` -> `*f(P, i, j)`, `P.col[i]` -> `*f(&P, i)`, and `&P.col[i]`
 * -> `f(&P, i)`: the `&*` collapses (rule 5). The path's bytes do not move, so
 * the renames of its names stay where they are. */
void k_em_edits_column(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->anchor < n->first || is->anchor >= n->end) continue;
        const char *sym;
        size_t sym_len, open, close;
        if (!column_at(ast, is, &sym, &sym_len, &open, &close)) continue;
        bool by_value = tok_is(ast, open - 2, ".");
        /* an `&` right before the path: it and the accessor's `*` cancel */
        bool addressed = is->anchor > n->first && tok_is(ast, is->anchor - 1, "&");
        size_t at = k_em_tok_off(ast, is->anchor);
        if (at >= a && at < b) {
            char *w = k_em_scratch(e, sym_len + 3);
            if (!w) return;
            size_t q = 0;
            if (!addressed) w[q++] = '*';
            memcpy(w + q, sym, sym_len); q += sym_len;
            w[q++] = '(';
            if (by_value) w[q++] = '&';
            size_t from = at, skip = 0;
            if (addressed) { from = k_em_tok_off(ast, is->anchor - 1); skip = 1; }
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = from, .skip = skip, .text = w, .text_len = q });
        }
        /* `->col[` (or `.col[`) becomes the argument separator */
        size_t sep = k_em_tok_end(ast, open - 3);
        if (sep >= a && sep < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = sep,
                .skip = k_em_tok_end(ast, open) - sep, .text = ", ", .text_len = 2 });
        at = k_em_tok_off(ast, close);
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = 1, .text = ")", .text_len = 1 });
    }
}
