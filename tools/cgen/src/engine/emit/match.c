/* engine/emit/match.c — the `match` of a tagged label (emit_internal.h; codegen design E5:
 * one file per construction; spec §4.9, backend §5.6). Lowered here: the operand is a
 * variable of type `tagged <Set> <T>`, read through the label field (`x->tag`, or `x.tag`
 * for a value); the body is a list of arms, each a label on a line of its own, and the
 * match's `{` and `}` end their lines. The dispatch is a `switch` of jumps; each arm is a
 * block behind its label, every one but the last followed by a jump to the end:
 *
 *   match (st) {            switch (st->tag) {
 *       ST1:                case ag_Cycle_ST1: goto keel__m0_ST1;
 *           ...             ...
 *           break;          default: KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
 *       ST2:                }
 *           ...             keel__m0_ST1: {
 *   }                           ...  goto keel__m0_end;
 *                           }
 *                           goto keel__m0_end;
 *                           keel__m0_ST2: {  ...  }
 *                           keel__m0_end: ;
 *
 * Consecutive labels stack before one brace. A `break` of the arm is a jump to the end.
 * The edits are text on the lines of the source: the `match` line becomes the dispatch
 * (several output lines, mapped on from the line), a label line becomes the close of the
 * previous arm and its own label (EDIT_LINES, so that the label gets its own `#line`),
 * and the closing `}` becomes the last arm's `}` and the end label.
 *
 * Refused (k_em_handles_match answers false, and the module is refused with
 * implementation-limit): an operand that is not a variable of a `tagged` type declared in
 * the function, a match whose labels share a line with other text, a `break` in an arm that
 * also has a loop, a `switch` or a nested `match`. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/names.h"

#define NO_TOKEN ((size_t)-1)
#define MAX_LABELS 32

typedef struct {
    size_t kw, open, close;         /* `match`, the match's `{` and `}` */
    size_t operand;                 /* the variable's token */
    bool ptr;                       /* declared with a `*` */
    char set[NAME_CAP];             /* the mangled set: `ag_Cycle` */
    size_t set_len;
    char disc[NAME_CAP];            /* what `switch` reads: `st->tag`, `s`, `keel_corot_tag(r)` */
    size_t disc_len;
    size_t label[MAX_LABELS], colon[MAX_LABELS];
    size_t n;
} Match;

static bool word(const KAst *ast, size_t i, const char *s) {
    return i < ast->tokens.len && k_em_text_is(k_em_tok(ast, i), s);
}

static bool ident(const KAst *ast, size_t i) {
    return i < ast->tokens.len && k_token_is_ident(k_em_tok(ast, i));
}

/* the start of the source line holding `off` */
static size_t line_start(const KAst *ast, size_t off) {
    while (off > 0 && ast->source.ptr[off - 1] != '\n') off--;
    return off;
}

/* only blanks between the start of the line and `off` */
static bool starts_line(const KAst *ast, size_t off) {
    for (size_t p = line_start(ast, off); p < off; p++)
        if (ast->source.ptr[p] != ' ' && ast->source.ptr[p] != '\t') return false;
    return true;
}

/* only blanks from `off` to the end of the line */
static bool ends_line(const KAst *ast, size_t off) {
    for (size_t p = off; p < ast->source.len && ast->source.ptr[p] != '\n'; p++)
        if (ast->source.ptr[p] != ' ' && ast->source.ptr[p] != '\t') return false;
    return true;
}

/* The set the labels come from and what `switch` reads (backend §5.6, rule 3), by the type in
 * the island, `st (tagged Cycle void): ST1 ST2`:
 *   - `tagged <Set> <T>`: the label field, `x.tag` or `x->tag`;
 *   - a type that is itself a set (`corot.Status`): the value, `x` or `*x`;
 *   - a type whose module declares `tag` (`corot`): the call of the verb, `keel_corot_tag(x)`,
 *     its set the one `tag` returns. */
static bool resolve_operand(const KAst *ast, const KIsland *is, Match *m, KToken name) {
    const char *t = keel_buffer_char_ptr(&ast->island_text, is->text);
    size_t len = is->text_len, p = 0;
    while (p < len && t[p] != '(') p++;
    size_t ts = p + 1, te = ts;
    while (te + 1 < len && !(t[te] == ')' && t[te + 1] == ':')) te++;
    if (p >= len || te + 1 >= len) return false;
    size_t q = 0;
    if (te - ts > 7 && memcmp(t + ts, "tagged ", 7) == 0) {
        size_t s = ts + 7, e = s;
        while (e < te && t[e] != ' ') e++;
        if (e == s) return false;
        for (size_t k = s; k < e; k++) if (t[k] == '.') return false;
        const KSymbol *sym = ast->symbols ? k_symbol_resolve(ast->symbols, (keel_slice_char){ e - s, (char *)t + s }) : NULL;
        if (!sym) return false;
        m->set_len = k_mangle_symbol(sym, (keel_slice_char){ sizeof m->set, m->set });
        if (m->set_len == 0 || m->set_len > sizeof m->set) return false;
        memcpy(m->disc, name.ptr, name.len);
        q = name.len;
        const char *f = m->ptr ? "->tag" : ".tag";
        memcpy(m->disc + q, f, strlen(f));
        m->disc_len = q + strlen(f);
        return true;
    }
    const char *sym;
    size_t sym_len;
    if (!ast->symbols || !k_em_type_symbol(ast, t + ts, te - ts, &sym, &sym_len)) return false;
    /* the written name (`corot.Status`) may be a set the table knows only by its qualified name */
    const KSymbol *by_name = k_symbol_resolve(ast->symbols, (keel_slice_char){ te - ts, (char *)t + ts });
    for (size_t i = 0; i < ast->symbols->len + 1; i++) {
        const KSymbol *s = i < ast->symbols->len ? keel_buffer_KSymbol_ptr(ast->symbols, i) : by_name;
        if (!s) continue;
        char ms[NAME_CAP];
        size_t ml = k_mangle_symbol(s, (keel_slice_char){ sizeof ms, ms });
        if (ml != sym_len || memcmp(ms, sym, ml) != 0) continue;
        if (s->kind == K_SYM_TAGS) {
            memcpy(m->set, ms, ml);
            m->set_len = ml;
            if (m->ptr) m->disc[q++] = '*';
            memcpy(m->disc + q, name.ptr, name.len);
            m->disc_len = q + name.len;
            return true;
        }
        if ((s->kind != K_SYM_TYPE && s->kind != K_SYM_MODIFIER) || !s->origin || !s->origin->ast) return false;
        const KAst *own = s->origin->ast;
        for (size_t n = 0; n < own->nodes.len; n++) {
            const KAstNode *fn = keel_buffer_KAstNode_ptr(&own->nodes, n);
            if (fn->kind != K_AST_FUNCTION || fn->name_first >= fn->name_end || !k_em_text_is(k_em_tok(own, fn->name_first), "tag")) continue;
            /* the return type: the word before the name; the parameter: by pointer or by value */
            size_t r = fn->first;
            while (r < fn->name_first && (k_em_is_visibility(k_em_tok(own, r)) || k_em_text_is(k_em_tok(own, r), "inline"))) r++;
            if (r + 1 != fn->name_first) return false;
            KToken ret = k_em_tok(own, r);
            bool vptr = false;
            for (size_t j = fn->name_first + 1; j < fn->body_first && !k_em_text_is(k_em_tok(own, j), ")"); j++)
                if (k_em_text_is(k_em_tok(own, j), "*")) vptr = true;
            size_t sl = k_mangle_decl(s->origin->name, (keel_slice_char){ ret.len, (char *)ret.ptr }, true, (keel_slice_char){ sizeof m->set, m->set });
            if (sl == 0 || sl > sizeof m->set) return false;
            m->set_len = sl;
            char verb[NAME_CAP];
            size_t vl = k_mangle_decl(s->origin->name, (keel_slice_char){ 3, (char *)"tag" }, false, (keel_slice_char){ sizeof verb, verb });
            if (vl > sizeof verb || vl + 4 + name.len + 2 > sizeof m->disc) return false;
            memcpy(m->disc, verb, vl);
            q = vl;
            m->disc[q++] = '(';
            if (vptr && !m->ptr) m->disc[q++] = '&';
            if (!vptr && m->ptr) m->disc[q++] = '*';
            memcpy(m->disc + q, name.ptr, name.len);
            q += name.len;
            m->disc[q++] = ')';
            m->disc_len = q;
            return true;
        }
        return false;
    }
    return false;
}

static bool analyze(const KAst *ast, const KIsland *is, Match *m) {
    if (is->kind != K_ISLAND_MATCH) return false;
    m->kw = is->anchor;
    if (!word(ast, m->kw, "match") || !word(ast, m->kw + 1, "(") || !ident(ast, m->kw + 2) || !word(ast, m->kw + 3, ")") ||
        !word(ast, m->kw + 4, "{"))
        return false;
    m->operand = m->kw + 2;
    m->open = m->kw + 4;
    size_t depth = 0;
    m->close = NO_TOKEN;
    for (size_t j = m->open; j < ast->tokens.len; j++) {
        if (word(ast, j, "{")) depth++;
        else if (word(ast, j, "}") && --depth == 0) { m->close = j; break; }
    }
    if (m->close == NO_TOKEN) return false;
    if (!ends_line(ast, k_em_tok_end(ast, m->open)) || !starts_line(ast, k_em_tok_off(ast, m->close)) ||
        !ends_line(ast, k_em_tok_end(ast, m->close)))
        return false;

    /* declared with a `*` or not: the first earlier use of the name that follows a type */
    KToken name = k_em_tok(ast, m->operand);
    m->ptr = false;
    bool found = false;
    for (size_t j = 0; j < m->kw && !found; j++) {
        KToken x = k_em_tok(ast, j);
        if (x.len != name.len || memcmp(x.ptr, name.ptr, x.len) != 0 || j == 0) continue;
        bool star = word(ast, j - 1, "*");
        /* a type word before the name: an identifier, or a C keyword such as `void` */
        KToken pv = k_em_tok(ast, j - 1);
        bool wordlike = pv.len && ((pv.ptr[0] >= 'a' && pv.ptr[0] <= 'z') || (pv.ptr[0] >= 'A' && pv.ptr[0] <= 'Z') || pv.ptr[0] == '_');
        if (!star && !wordlike) continue;
        m->ptr = star;
        found = true;
    }
    if (!found) return false;
    if (!resolve_operand(ast, is, m, name)) return false;

    /* the labels of the arms, at depth 1 of the match's braces */
    m->n = 0;
    depth = 0;
    for (size_t j = m->open + 1; j < m->close; j++) {
        if (word(ast, j, "{") || word(ast, j, "(") || word(ast, j, "[")) depth++;
        else if (word(ast, j, "}") || word(ast, j, ")") || word(ast, j, "]")) depth--;
        else if (!depth && ident(ast, j) && word(ast, j + 1, ":") &&
                 (word(ast, j - 1, "{") || word(ast, j - 1, ";") || word(ast, j - 1, "}") || word(ast, j - 1, ":"))) {
            if (m->n == MAX_LABELS) return false;
            /* a label has its line to itself */
            if (!starts_line(ast, k_em_tok_off(ast, j)) || !ends_line(ast, k_em_tok_end(ast, j + 1))) return false;
            m->label[m->n] = j;
            m->colon[m->n++] = j + 1;
        }
    }
    if (m->n == 0) return false;

    /* a `break` of the arm is a jump to the end: only when no inner construct could own it */
    bool brk = false, inner = false;
    for (size_t j = m->open + 1; j < m->close; j++) {
        if (word(ast, j, "break")) brk = true;
        if (word(ast, j, "for") || word(ast, j, "while") || word(ast, j, "do") || word(ast, j, "switch") ||
            word(ast, j, "foreach") || word(ast, j, "match"))
            inner = true;
    }
    return !(brk && inner);
}

bool k_em_handles_match(const KAst *ast, const KIsland *is) {
    Match m;
    return is->kind == K_ISLAND_MATCH && analyze(ast, is, &m);
}

static size_t put(char *w, size_t q, const char *s, size_t n) {
    memcpy(w + q, s, n);
    return q + n;
}

static size_t put_uint(char *w, size_t q, size_t v) {
    char rev[24];
    size_t d = 0;
    do { rev[d++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (d) w[q++] = rev[--d];
    return q;
}

/* appends x when its position is in the line [a, b) */
static void push_in_line(Emit *e, Edit *ed, size_t *cnt, size_t a, size_t b, Edit x) {
    if (x.at >= a && x.at < b) k_em_push_edit(e, ed, cnt, x);
}

void k_em_edits_match(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    const char *src = ast->source.ptr;
    size_t ordinal = 0;
    for (size_t k = 0; k < ast->islands.len && e->ok; k++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, k);
        Match m;
        if (is->kind != K_ISLAND_MATCH || is->anchor < fn->first || is->anchor >= fn->end || !analyze(ast, is, &m)) continue;
        size_t N = ordinal++;
        size_t kw_off = k_em_tok_off(ast, m.kw), close_off = k_em_tok_off(ast, m.close);
        if (close_off < a && kw_off < a && !(m.close >= fn->first)) continue;
        /* the indentation of the match */
        size_t ind0 = line_start(ast, kw_off), ind_n = kw_off - ind0;
        const char *ind = src + ind0;
        char id[16];
        size_t idn = put_uint(id, 0, N);

        /* labels of the arm groups: [first, last] indices into m.label */
        size_t ln_of_a = k_em_line_at(e, a);

        /* the dispatch, in place of `match (x) {` */
        if (kw_off >= a && kw_off < b) {
            size_t width = 8;                                       /* `default:` */
            for (size_t i = 0; i < m.n; i++) {
                size_t w = 5 + m.set_len + 1 + k_em_tok(ast, m.label[i]).len + 1;
                if (w > width) width = w;
            }
            size_t need = 64 + m.n * (ind_n + width + 64 + 2 * (m.set_len + 16)) + 3 * ind_n + 128;
            char *w = k_em_scratch(e, need);
            if (!w) return;
            size_t q = 0;
            q = put(w, q, "switch (", sizeof "switch (" - 1);
            q = put(w, q, m.disc, m.disc_len);
            q = put(w, q, ") {", 3);
            for (size_t i = 0; i < m.n; i++) {
                KToken tag = k_em_tok(ast, m.label[i]);
                q = put(w, q, "\n", sizeof "\n" - 1);
                q = put(w, q, ind, ind_n);
                q = put(w, q, "case ", sizeof "case " - 1);
                q = put(w, q, m.set, m.set_len);
                q = put(w, q, "_", sizeof "_" - 1);
                q = put(w, q, tag.ptr, tag.len);
                q = put(w, q, ":", sizeof ":" - 1);
                size_t used = 5 + m.set_len + 1 + tag.len + 1;
                for (size_t sp = used; sp < width + 1; sp++) w[q++] = ' ';
                q = put(w, q, "goto keel__m", sizeof "goto keel__m" - 1);
                q = put(w, q, id, idn);
                q = put(w, q, "_", sizeof "_" - 1);
                q = put(w, q, tag.ptr, tag.len);
                q = put(w, q, ";", sizeof ";" - 1);
            }
            q = put(w, q, "\n", sizeof "\n" - 1);
            q = put(w, q, ind, ind_n);
            q = put(w, q, "default:", sizeof "default:" - 1);
            for (size_t sp = 8; sp < width + 1; sp++) w[q++] = ' ';
            q = put(w, q, "KEEL_CHECK(0, \"tag-out-of-range\"); goto keel__m", sizeof "KEEL_CHECK(0, \"tag-out-of-range\"); goto keel__m" - 1);
            q = put(w, q, id, idn);
            q = put(w, q, "_end;\n", sizeof "_end;\n" - 1);
            q = put(w, q, ind, ind_n);
            q = put(w, q, "}", sizeof "}" - 1);
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = kw_off, .skip = k_em_tok_end(ast, m.open) - kw_off,
                .text = w, .text_len = q });
        }

        /* the arms are one level out: the body loses what the label has over the `match` */
        size_t lab0 = line_start(ast, k_em_tok_off(ast, m.label[0]));
        size_t delta = 0;
        while (lab0 + delta < ast->source.len && src[lab0 + delta] == ' ') delta++;
        delta = delta > ind_n ? delta - ind_n : 0;
        if (delta && a > lab0 && a < close_off && ast->source.ptr[a] != '\n') {
            bool label_line = false;
            for (size_t i = 0; i < m.n; i++) label_line = label_line || line_start(ast, k_em_tok_off(ast, m.label[i])) == a;
            size_t lead = 0;
            while (a + lead < b && src[a + lead] == ' ') lead++;
            if (a == line_start(ast, close_off)) label_line = true;
            if (!label_line && lead >= delta && a + lead < b && src[a + lead] != '\n')
                push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_DROP, .at = a, .skip = delta });
        }

        /* the labels */
        for (size_t i = 0; i < m.n; i++) {
            size_t first_off = k_em_tok_off(ast, m.label[i]);
            if (first_off < a || first_off >= b) continue;
            bool stacked_after = i + 1 < m.n && m.label[i + 1] == m.colon[i] + 1;   /* the next label follows at once */
            bool starts_group = i == 0 || m.label[i] != m.colon[i - 1] + 1;
            KToken tag = k_em_tok(ast, m.label[i]);
            size_t from = line_start(ast, first_off), to = k_em_tok_end(ast, m.colon[i]);
            size_t need = 3 * ind_n + 2 * tag.len + 128;
            char *w = k_em_scratch(e, need);
            if (!w) return;
            size_t q = 0;
            if (starts_group && i > 0) {
                /* the close of the previous arm, as lines of their own; the label then gets its `#line` */
                q = put(w, q, "L", sizeof "L" - 1);
                q = put_uint(w, q, ln_of_a);
                q = put(w, q, "|", sizeof "|" - 1);
                q = put(w, q, ind, ind_n);
                q = put(w, q, "}\nL", sizeof "}\nL" - 1);
                q = put_uint(w, q, ln_of_a + 1);
                q = put(w, q, "|", sizeof "|" - 1);
                q = put(w, q, ind, ind_n);
                q = put(w, q, "goto keel__m", sizeof "goto keel__m" - 1);
                q = put(w, q, id, idn);
                q = put(w, q, "_end;\nT|", sizeof "_end;\nT|" - 1);
            }
            q = put(w, q, ind, ind_n);
            q = put(w, q, "keel__m", sizeof "keel__m" - 1);
            q = put(w, q, id, idn);
            q = put(w, q, "_", sizeof "_" - 1);
            q = put(w, q, tag.ptr, tag.len);
            q = stacked_after ? put(w, q, ":", 1) : put(w, q, ": {", 3);
            if (starts_group && i > 0)
                push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_LINES, .at = from, .skip = to - from, .text = w, .text_len = q });
            else
                push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = from, .skip = to - from, .text = w, .text_len = q });
        }

        /* the arm's `break` is the jump to the end */
        for (size_t j = m.open + 1; j < m.close; j++) {
            if (!word(ast, j, "break")) continue;
            size_t off = k_em_tok_off(ast, j);
            if (off < a || off >= b) continue;
            char *w = k_em_scratch(e, 32);
            if (!w) return;
            size_t q = put(w, 0, "goto keel__m", sizeof "goto keel__m" - 1);
            q = put(w, q, id, idn);
            q = put(w, q, "_end", sizeof "_end" - 1);
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = off, .skip = 5, .text = w, .text_len = q });
        }

        /* the closing `}`: the last arm's, and the end label */
        if (close_off >= a && close_off < b) {
            char *w = k_em_scratch(e, 2 * ind_n + 48);
            if (!w) return;
            size_t q = put(w, 0, "}\n", sizeof "}\n" - 1);
            q = put(w, q, ind, ind_n);
            q = put(w, q, "keel__m", sizeof "keel__m" - 1);
            q = put(w, q, id, idn);
            q = put(w, q, "_end: ;", sizeof "_end: ;" - 1);
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = close_off, .skip = 1, .text = w, .text_len = q });
        }
    }
}
