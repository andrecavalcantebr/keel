/* engine/emit/parallel.c — the `parallel` block and the exits of its worker (emit_internal.h;
 * codegen design E5: one file per construction; spec §4.8, backend §5.9). The header line
 *
 *   parallel NAME POLICY (size_t w : 0..K; T part : C [; (captures)]) {
 *
 * becomes the control symbol, the container's temporary and the worker loop, on the lines
 * the header and the block's closing `}` held; the body keeps its lines, one level deeper:
 *
 *   keel_parallel_control NAME = { .workers = K, .target = T };
 *   {   S *keel__c<N> = C;
 *
 *       #pragma omp parallel for num_threads(K) default(none) \
 *               shared(keel__c<N>, NAME, ...) firstprivate(...)
 *       for (size_t w = 0; w < K; w++) {
 *           T part = S_partition(keel__c<N>, K, w);
 *           body
 *           keel__end<M>: ;
 *       }
 *   }
 *
 * `win;` and `fail;` write the control symbol and jump to the label; `parallel.interrupted(NAME)`
 * reads the flag where it stands (rules 8 to 11). The directive is always written: without
 * `-fopenmp` the C compiler ignores it (rule 5.9.1.2).
 *
 * Refused (k_em_handles_parallel answers false): a header over several lines, a `{` that does
 * not end its line, workers other than `0..K` with a decimal K, a policy that is not ALL, ANY or
 * a decimal, a partition binder with more than one name, a capture not declared in the function. */
#include <string.h>
#include "engine/emit/emit_internal.h"

#define NO_TOKEN ((size_t)-1)
#define MAX_CAPS 16

typedef struct {
    size_t kw, name, policy;
    size_t open;                    /* the header's `(` */
    size_t wtype_a, wtype_b, wname; /* `size_t w` */
    size_t zero, dots, count;       /* `0 .. K` */
    size_t ptype_a, ptype_b, pname; /* `T part` */
    size_t cont;                    /* the container variable */
    size_t caps[MAX_CAPS], ncaps;
    size_t brace, close;            /* the body's `{` and `}` */
    const char *sym; size_t sym_len;        /* the container's instance */
    const char *part; size_t part_len;      /* the partition verb's symbol */
    bool by_pointer;
    size_t target;
} Par;

static bool word(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

static bool ident(const KAst *ast, size_t i) {
    return i < ast->tokens.len && k_token_is_ident(k_em_tok(ast, i));
}

static size_t line_start(const KAst *ast, size_t off) {
    while (off > 0 && ast->source.ptr[off - 1] != '\n') off--;
    return off;
}

static bool ends_line(const KAst *ast, size_t off) {
    for (size_t p = off; p < ast->source.len && ast->source.ptr[p] != '\n'; p++)
        if (ast->source.ptr[p] != ' ' && ast->source.ptr[p] != '\t') return false;
    return true;
}

static bool starts_line(const KAst *ast, size_t off) {
    for (size_t p = line_start(ast, off); p < off; p++)
        if (ast->source.ptr[p] != ' ' && ast->source.ptr[p] != '\t') return false;
    return true;
}

static bool decimal(KToken t) {
    if (!t.len) return false;
    for (size_t i = 0; i < t.len; i++) if (t.ptr[i] < '0' || t.ptr[i] > '9') return false;
    return true;
}

static bool analyze(const KAst *ast, const KIsland *is, const KAstNode *fn, Par *p) {
    if (is->kind != K_ISLAND_PARALLEL) return false;
    p->kw = is->anchor;
    if (!word(ast, p->kw, "parallel") || !ident(ast, p->kw + 1) || !word(ast, p->kw + 3, "(")) return false;
    p->name = p->kw + 1;
    p->policy = p->kw + 2;
    p->open = p->kw + 3;
    /* the policy is the target: ALL is 0 (nobody fails), ANY is 1, a decimal N is N wins */
    KToken pol = k_em_tok(ast, p->policy);
    p->target = 0;
    if (k_em_text_is(pol, "ALL")) p->target = 0;
    else if (k_em_text_is(pol, "ANY")) p->target = 1;
    else if (decimal(pol)) for (size_t i = 0; i < pol.len; i++) p->target = p->target * 10 + (size_t)(pol.ptr[i] - '0');
    else return false;       /* a constant by name is not lowered yet */

    /* `size_t w : 0 .. K ;` */
    size_t c1 = NO_TOKEN;
    for (size_t i = p->open + 1; i < ast->tokens.len && c1 == NO_TOKEN; i++) {
        if (word(ast, i, ":")) c1 = i;
        else if (word(ast, i, ";") || word(ast, i, "{")) return false;
    }
    if (c1 == NO_TOKEN || c1 < p->open + 3 || !ident(ast, c1 - 1)) return false;
    p->wtype_a = p->open + 1; p->wtype_b = c1 - 1; p->wname = c1 - 1;
    p->zero = c1 + 1; p->dots = c1 + 2; p->count = c1 + 3;
    if (!word(ast, p->zero, "0") || !word(ast, p->dots, "..") || !decimal(k_em_tok(ast, p->count)) || !word(ast, p->count + 1, ";")) return false;

    /* `T part : C` */
    size_t c2 = NO_TOKEN;
    for (size_t i = p->count + 2; i < ast->tokens.len && c2 == NO_TOKEN; i++) {
        if (word(ast, i, ":")) c2 = i;
        else if (word(ast, i, ";") || word(ast, i, "{") || word(ast, i, ")")) return false;
    }
    if (c2 == NO_TOKEN || c2 < p->count + 4 || !ident(ast, c2 - 1) || !ident(ast, c2 + 1)) return false;
    p->ptype_a = p->count + 2; p->ptype_b = c2 - 1; p->pname = c2 - 1;
    p->cont = c2 + 1;

    /* `)` or `; ( a , b ) )` */
    size_t j = p->cont + 1;
    p->ncaps = 0;
    if (word(ast, j, ";")) {
        if (!word(ast, j + 1, "(")) return false;
        j += 2;
        while (!word(ast, j, ")")) {
            if (!ident(ast, j) || p->ncaps == MAX_CAPS) return false;
            p->caps[p->ncaps++] = j++;
            if (word(ast, j, ",")) j++;
            else if (!word(ast, j, ")")) return false;
        }
        j++;
    }
    if (!word(ast, j, ")") || !word(ast, j + 1, "{")) return false;
    p->brace = j + 1;
    if (!ends_line(ast, k_em_tok_end(ast, p->brace)) || !starts_line(ast, k_em_tok_off(ast, p->kw))) return false;
    size_t depth = 0;
    p->close = NO_TOKEN;
    for (size_t i = p->brace; i < ast->tokens.len; i++) {
        if (word(ast, i, "{")) depth++;
        else if (word(ast, i, "}") && --depth == 0) { p->close = i; break; }
    }
    if (p->close == NO_TOKEN || !starts_line(ast, k_em_tok_off(ast, p->close)) || !ends_line(ast, k_em_tok_end(ast, p->close))) return false;

    /* the island: `NAME POLICY (w : 0..K; part : C (<type>)[; (caps)]) → <partition symbol>` */
    const char *t = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, arrow = len;
    static const char ARROW[] = "\xE2\x86\x92";
    for (size_t k = 0; k + 3 <= len; k++) if (memcmp(t + k, ARROW, 3) == 0) { arrow = k; break; }
    if (arrow == len) return false;
    size_t s = arrow + 3;
    while (s < len && t[s] == ' ') s++;
    p->part = t + s; p->part_len = len - s;
    if (!p->part_len) return false;
    KToken cont = k_em_tok(ast, p->cont);
    size_t q = 0;
    for (; q + cont.len + 2 < arrow; q++)
        if (memcmp(t + q, cont.ptr, cont.len) == 0 && t[q + cont.len] == ' ' && t[q + cont.len + 1] == '(') break;
    if (q + cont.len + 2 >= arrow) return false;
    size_t ts = q + cont.len + 2, te = ts, depth2 = 1;
    while (te < arrow && depth2) { if (t[te] == '(') depth2++; else if (t[te] == ')') depth2--; te++; }
    if (depth2) return false;
    if (!k_em_type_symbol(ast, t + ts, te - 1 - ts, &p->sym, &p->sym_len)) return false;

    /* by value or by pointer: the declaration of the container in the function */
    KToken c = k_em_tok(ast, p->cont);
    bool found = false;
    for (size_t i = fn->first + 1; i < p->kw && !found; i++) {
        KToken x = k_em_tok(ast, i);
        if (x.len != c.len || memcmp(x.ptr, c.ptr, c.len) != 0) continue;
        if (word(ast, i - 1, "*")) { p->by_pointer = true; found = true; }
        else if (ident(ast, i - 1)) { p->by_pointer = false; found = true; }
    }
    if (!found) return false;
    /* each capture must be declared too */
    for (size_t k = 0; k < p->ncaps; k++) {
        KToken x = k_em_tok(ast, p->caps[k]);
        bool d = false;
        for (size_t i = fn->first + 1; i < p->kw && !d; i++) {
            KToken y = k_em_tok(ast, i);
            d = y.len == x.len && memcmp(y.ptr, x.ptr, y.len) == 0 && (word(ast, i - 1, "*") || ident(ast, i - 1));
        }
        if (!d) return false;
    }
    return true;
}

static const KAstNode *fn_of(const KAst *ast, size_t tok) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind == K_AST_FUNCTION && n->body_first != NO_TOKEN && n->body_end != NO_TOKEN && tok >= n->body_first && tok < n->body_end)
            return n;
    }
    return NULL;
}

bool k_em_handles_parallel(const KAst *ast, const KIsland *is) {
    Par p;
    const KAstNode *fn = is->kind == K_ISLAND_PARALLEL ? fn_of(ast, is->anchor) : NULL;
    return fn && analyze(ast, is, fn, &p);
}

/* the parallel block whose body holds token `tok`, or false; `n` is its ordinal in the function */
static bool block_of(const KAst *ast, size_t tok, Par *out, size_t *ordinal) {
    const KAstNode *fn = fn_of(ast, tok);
    if (!fn) return false;
    size_t n = 0;
    for (size_t k = 0; k < ast->islands.len; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        Par p;
        if (is->kind != K_ISLAND_PARALLEL || is->anchor < fn->body_first || is->anchor >= fn->body_end || !analyze(ast, is, fn, &p)) continue;
        if (tok > p.brace && tok < p.close) { *out = p; *ordinal = n; return true; }
        n++;
    }
    return false;
}

static size_t put(char *w, size_t q, const char *s, size_t n) {
    memcpy(w + q, s, n);
    return q + n;
}
#define LIT(w, q, s) put(w, q, s, sizeof s - 1)

static size_t put_uint(char *w, size_t q, size_t v) {
    char rev[24];
    size_t d = 0;
    do { rev[d++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (d) w[q++] = rev[--d];
    return q;
}

static size_t spaces(char *w, size_t q, size_t n) {
    while (n--) w[q++] = ' ';
    return q;
}

static void push_in_line(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, Edit x) {
    if (x.at >= a && x.at < b) k_em_push_edit(e, ed, cnt, x);
}

/* the declared type of a captured variable is a pointer: it is shared, else copied */
static bool captured_pointer(const KAst *ast, const KAstNode *fn, size_t before, KToken x) {
    for (size_t i = fn->first + 1; i < before; i++) {
        KToken y = k_em_tok(ast, i);
        if (y.len == x.len && memcmp(y.ptr, x.ptr, y.len) == 0 && word(ast, i - 1, "*")) return true;
    }
    return false;
}

void k_em_edits_parallel(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    const char *src = ast->source.ptr;
    size_t ordinal = 0;
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        Par p;
        if (is->kind != K_ISLAND_PARALLEL || is->anchor < fn->body_first || is->anchor >= fn->body_end || !analyze(ast, is, fn, &p)) continue;
        size_t M = ordinal++;
        size_t N = k_em_capture_ordinal(ast, fn, p.kw);
        size_t kw_off = k_em_tok_off(ast, p.kw), close_off = k_em_tok_off(ast, p.close);
        size_t ind_n = kw_off - line_start(ast, kw_off);
        size_t W = 0;
        for (size_t i = 0; i < k_em_tok(ast, p.count).len; i++) W = W * 10 + (size_t)(k_em_tok(ast, p.count).ptr[i] - '0');
        KToken name = k_em_tok(ast, p.name);

        if (kw_off >= a && kw_off < b) {
            e->n = 0;
            char *w = k_em_scratch(e, 1000);
            if (!w) return;
            size_t q = 0;
            q = LIT(w, q, "keel_parallel_control ");
            q = put(w, q, name.ptr, name.len);
            q = LIT(w, q, " = { .workers = ");
            q = put_uint(w, q, W);
            q = LIT(w, q, ", .target = ");
            q = put_uint(w, q, p.target);
            q = LIT(w, q, " };\n");
            q = spaces(w, q, ind_n);
            q = LIT(w, q, "{   ");
            q = put(w, q, p.sym, p.sym_len);
            q = LIT(w, q, " *keel__c");
            q = put_uint(w, q, N);
            q = LIT(w, q, " = ");
            if (!p.by_pointer) q = LIT(w, q, "&");
            KToken cont = k_em_tok(ast, p.cont);
            q = put(w, q, cont.ptr, cont.len);
            q = LIT(w, q, ";\n\n");
            size_t directive = q;   /* where the directive starts: --parallel-lowering=serie drops it */
            q = spaces(w, q, ind_n + 4);
            q = LIT(w, q, "#pragma omp parallel for num_threads(");
            q = put_uint(w, q, W);
            q = LIT(w, q, ") default(none) \\\n");
            q = spaces(w, q, ind_n + 12);
            q = LIT(w, q, "shared(keel__c");
            q = put_uint(w, q, N);
            q = LIT(w, q, ", ");
            q = put(w, q, name.ptr, name.len);
            for (size_t c = 0; c < p.ncaps; c++)
                if (captured_pointer(ast, fn, p.kw, k_em_tok(ast, p.caps[c]))) {
                    q = LIT(w, q, ", ");
                    q = put(w, q, k_em_tok(ast, p.caps[c]).ptr, k_em_tok(ast, p.caps[c]).len);
                }
            q = LIT(w, q, ")");
            bool first = true;
            for (size_t c = 0; c < p.ncaps; c++)
                if (!captured_pointer(ast, fn, p.kw, k_em_tok(ast, p.caps[c]))) {
                    q = first ? LIT(w, q, " firstprivate(") : LIT(w, q, ", ");
                    first = false;
                    q = put(w, q, k_em_tok(ast, p.caps[c]).ptr, k_em_tok(ast, p.caps[c]).len);
                }
            if (!first) q = LIT(w, q, ")");
            q = LIT(w, q, "\n");
            if (!e->opt.omp) q = directive;
            q = spaces(w, q, ind_n + 4);
            q = LIT(w, q, "for (");
            /* the worker's binder is written as the source has it */
            e->n = 0;
            k_em_put_range(e, p.wtype_a, p.wtype_b);
            if (!e->ok || q + e->n + 200 > 1000) { e->ok = false; return; }
            q = put(w, q, e->buf, e->n);
            e->n = 0;
            KToken wn = k_em_tok(ast, p.wname);
            q = LIT(w, q, " ");
            q = put(w, q, wn.ptr, wn.len);
            q = LIT(w, q, " = 0; ");
            q = put(w, q, wn.ptr, wn.len);
            q = LIT(w, q, " < ");
            q = put_uint(w, q, W);
            q = LIT(w, q, "; ");
            q = put(w, q, wn.ptr, wn.len);
            q = LIT(w, q, "++) {\n");
            q = spaces(w, q, ind_n + 8);
            k_em_put_range(e, p.ptype_a, p.ptype_b);
            if (!e->ok || q + e->n + 300 > 1000) { e->ok = false; return; }
            q = put(w, q, e->buf, e->n);
            e->n = 0;
            KToken pn = k_em_tok(ast, p.pname);
            q = LIT(w, q, " ");
            q = put(w, q, pn.ptr, pn.len);
            q = LIT(w, q, " = ");
            q = put(w, q, p.part, p.part_len);
            q = LIT(w, q, "(keel__c");
            q = put_uint(w, q, N);
            q = LIT(w, q, ", ");
            q = put_uint(w, q, W);
            q = LIT(w, q, ", ");
            q = put(w, q, wn.ptr, wn.len);
            q = LIT(w, q, ");");
            if (q > 1000) { e->ok = false; return; }
            /* the header's tokens were renamed by the module's rules: they are all replaced */
            size_t end = k_em_tok_end(ast, p.brace), j = 0;
            for (size_t c = 0; c < *cnt; c++)
                if (ed[c].at < kw_off || ed[c].at >= end) ed[j++] = ed[c];
            *cnt = j;
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = kw_off, .skip = end - kw_off, .text = w, .text_len = q });
        }

        /* the body is one level deeper */
        size_t head_line = line_start(ast, kw_off), close_line = line_start(ast, close_off);
        if (a > head_line && a < close_line && a < b && src[a] != '\n') {
            size_t lead = 0;
            while (a + lead < b && (src[a + lead] == ' ' || src[a + lead] == '\t')) lead++;
            if (a + lead < b && src[a + lead] != '\n')
                push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = a, .skip = 0, .text = "    ", .text_len = 4 });
        }

        /* the block's `}`: the label that ends a worker, and the closes of the loop and of the block */
        if (close_off >= a && close_off < b) {
            char *w = k_em_scratch(e, 3 * ind_n + 64);
            if (!w) return;
            size_t q = spaces(w, 0, 8);
            q = LIT(w, q, "keel__end");
            q = put_uint(w, q, M);
            q = LIT(w, q, ": ;\n");
            q = spaces(w, q, ind_n + 4);
            q = LIT(w, q, "}\n");
            q = spaces(w, q, ind_n);
            q = LIT(w, q, "}");
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = close_off, .skip = 1, .text = w, .text_len = q });
        }
    }
}

bool k_em_handles_worker_exit(const KAst *ast, const KIsland *is) {
    Par p;
    size_t M;
    return is->kind == K_ISLAND_WORKER_EXIT && word(ast, is->anchor + 1, ";") && block_of(ast, is->anchor, &p, &M);
}

/* `win;` and `fail;` of a worker (rule 8), and `parallel.interrupted(NAME)` (rule 11) */
void k_em_edits_worker_exit(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        if (is->kind != K_ISLAND_WORKER_EXIT || is->anchor < fn->body_first || is->anchor >= fn->body_end) continue;
        size_t kw = is->anchor;
        if (!word(ast, kw + 1, ";") || k_em_tok_off(ast, kw) < a || k_em_tok_off(ast, kw) >= b) continue;
        Par p;
        size_t M;
        if (!block_of(ast, kw, &p, &M)) { e->ok = false; return; }
        KToken name = k_em_tok(ast, p.name);
        bool win = word(ast, kw, "win");
        bool brace = word(ast, kw - 1, ")") || word(ast, kw - 1, "else");
        char *w = k_em_scratch(e, 400);
        if (!w) return;
        size_t q = 0;
        if (brace) q = LIT(w, q, "{ ");
        if (!win) {
            q = LIT(w, q, "atomic_fetch_add_explicit(&");
            q = put(w, q, name.ptr, name.len);
            q = LIT(w, q, ".fails, 1, memory_order_relaxed);");
        } else if (p.target == 0) {
            q = LIT(w, q, "atomic_fetch_add_explicit(&");
            q = put(w, q, name.ptr, name.len);
            q = LIT(w, q, ".wins, 1, memory_order_relaxed);");
        } else {
            q = LIT(w, q, "if (atomic_fetch_add_explicit(&");
            q = put(w, q, name.ptr, name.len);
            q = LIT(w, q, ".wins, 1, memory_order_relaxed) + 1 >= ");
            q = put_uint(w, q, p.target);
            q = LIT(w, q, ") atomic_store_explicit(&");
            q = put(w, q, name.ptr, name.len);
            q = LIT(w, q, ".flag, true, memory_order_relaxed);");
        }
        q = LIT(w, q, " goto keel__end");
        q = put_uint(w, q, M);
        q = LIT(w, q, ";");
        if (brace) q = LIT(w, q, " }");
        k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, kw),
            .skip = k_em_tok_end(ast, kw + 1) - k_em_tok_off(ast, kw), .text = w, .text_len = q });
    }
}

/* `parallel.interrupted(NAME)` in a worker: the flag read where it stands. True when the
 * call island `is` was written (uses.c then leaves it alone). */
bool k_em_parallel_interrupted(Emit *e, const KIsland *is, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    if (is->kind != K_ISLAND_CALL) return false;
    const char *t = keel_buffer_char_ptr(&ast->island_text, is->text);
    if (is->text_len < 20 || memcmp(t, "parallel.interrupted", 20) != 0) return false;
    Par p;
    size_t M;
    if (!block_of(ast, is->anchor, &p, &M)) return false;
    size_t open = is->anchor;
    while (open < ast->tokens.len && !word(ast, open, "(")) open++;
    if (!ident(ast, open + 1) || !word(ast, open + 2, ")")) return false;
    KToken name = k_em_tok(ast, open + 1);
    size_t at = k_em_tok_off(ast, is->anchor);
    if (at < a || at >= b) return true;
    char *w = k_em_scratch(e, name.len + 64);
    if (!w) return true;
    size_t q = LIT(w, 0, "atomic_load_explicit(&");
    q = put(w, q, name.ptr, name.len);
    q = LIT(w, q, ".flag, memory_order_relaxed)");
    k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_TEXT, .at = at, .skip = k_em_tok_end(ast, open + 2) - at, .text = w, .text_len = q });
    return true;
}
