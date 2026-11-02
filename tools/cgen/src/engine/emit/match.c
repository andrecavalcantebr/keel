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
 * Consecutive labels stack before one brace. A `break` of the arm is a jump to the end;
 * a `break` inside a loop, `switch` or `match` of the arm belongs to that construct and
 * stays. An arm whose instruction shares the label's line is closed at the end of that
 * line (`A: { r = 1; }`), and its own line is the whole arm.
 * The edits are text on the lines of the source: the `match` line becomes the dispatch
 * (several output lines, mapped on from the line), a label line becomes the close of the
 * previous arm and its own label (EDIT_LINES, so that the label gets its own `#line`),
 * and the closing `}` becomes the last arm's `}` and the end label.
 *
 * Refused (k_em_handles_match answers false, and the module is refused with
 * implementation-limit): an operand that is not a variable of a `tagged` type declared in
 * the function, or a field `x->f` / `x.f` whose struct and field this module does not
 * declare as `tagged <Set> <T>`; a label whose instruction goes on to the next line; a
 * match whose labels share a line with other text. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/names.h"

#define NO_TOKEN ((size_t)-1)
#define MAX_LABELS 32

typedef struct {
    size_t kw, open, close;         /* `match`, the match's `{` and `}` */
    size_t operand;                 /* the variable's token, or the first of a column access */
    size_t rp;                      /* the `)` after the operand */
    bool column;                    /* the operand is `P->col[i]` of an extent: its text stays, lowered by extent.c */
    bool ptr;                       /* declared with a `*` */
    char set[NAME_CAP];             /* the mangled set: `ag_Cycle` */
    size_t set_len;
    char disc[NAME_CAP];            /* what `switch` reads: `st->tag`, `s`, `keel_corot_tag(r)` */
    size_t disc_len;
    bool field;                     /* the operand is `x->f` or `x.f`: the field of a struct of this module */
    size_t label[MAX_LABELS], colon[MAX_LABELS];
    bool inl[MAX_LABELS];           /* the arm's instruction shares the label's line */
    size_t n;
} Match;

/* the walk of break_owner: the open braces of the arm, the constructs among them */
#define BLOCK_CAP 32
#define BREAK_OWN 0
#define BREAK_ARM 1
#define BREAK_DEEP 2

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

/* the offset of the '\n' that ends the line holding `off` (the source's end if none) */
static size_t line_end(const KAst *ast, size_t off) {
    while (off < ast->source.len && ast->source.ptr[off] != '\n') off++;
    return off;
}

/* a word of the keel text, keywords included (`void` is one): a letter or `_` first */
static bool word_like(const KAst *ast, size_t i) {
    KToken t = k_em_tok(ast, i);
    return t.len && ((t.ptr[0] >= 'a' && t.ptr[0] <= 'z') || (t.ptr[0] >= 'A' && t.ptr[0] <= 'Z') || t.ptr[0] == '_');
}

static bool same_text(KToken a, KToken b) {
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
}

/* the name token of the declaration of `name` in the function that holds the match at kw: the
 * first earlier use of the name that follows a type or a `*` (`Holder *h`) */
static bool decl_of(const KAst *ast, size_t kw, KToken name, size_t *out) {
    size_t from = 0;
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *fn = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (fn->kind == K_AST_FUNCTION && kw >= fn->first && kw < fn->end) from = fn->first;
    }
    for (size_t j = from ? from : 1; j < kw; j++) {
        KToken x = k_em_tok(ast, j);
        if (x.len != name.len || memcmp(x.ptr, name.ptr, x.len) != 0 || j == 0) continue;
        bool star = word(ast, j - 1, "*");
        /* a type word before the name: an identifier, or a C keyword such as `void` */
        if (!star && !word_like(ast, j - 1)) continue;
        *out = j;
        return true;
    }
    return false;
}

/* The field operand `h->state` (or `h.state`), backend §5.6 rule 3: the type of `h` is a struct
 * declared in this module (`struct Holder` or `typedef struct {...} Holder`), and its field
 * `state` is `tagged <Set> <T>`, by value or by pointer. What switch reads is the operand as
 * written, with the field's label: `h->state.tag`, or `->tag` for a pointer field. */
static bool field_operand(const KAst *ast, Match *m) {
    KToken var = k_em_tok(ast, m->operand), op = k_em_tok(ast, m->kw + 3), fld = k_em_tok(ast, m->kw + 4);
    size_t dj;
    if (!decl_of(ast, m->kw, var, &dj) || dj < 2) return false;
    size_t tt = word(ast, dj - 1, "*") ? dj - 2 : dj - 1;
    /* a qualified name (`other.Holder`) is a type of another module: not this module's */
    if (tt == 0 || !word_like(ast, tt) || word(ast, tt - 1, ".")) return false;
    KToken tname = k_em_tok(ast, tt);
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_TYPE || n->name_end != n->name_first + 1 || !same_text(k_em_tok(ast, n->name_first), tname)) continue;
        size_t o = NO_TOKEN, c = NO_TOKEN, depth = 0, s = NO_TOKEN;
        for (size_t j = n->first; j < n->end && o == NO_TOKEN; j++) {
            if (word(ast, j, "struct")) s = j;
            if (word(ast, j, "{")) o = j;
        }
        if (o == NO_TOKEN || s == NO_TOKEN) return false;
        for (size_t j = o; j < n->end && c == NO_TOKEN; j++) {
            if (word(ast, j, "{")) depth++;
            else if (word(ast, j, "}") && --depth == 0) c = j;
        }
        if (c == NO_TOKEN) return false;
        /* the fields at the struct's own level: `tagged <Set> <T> [*]name;` */
        depth = 0;
        for (size_t j = o + 1; j < c; j++) {
            if (word(ast, j, "{") || word(ast, j, "(") || word(ast, j, "[")) depth++;
            else if (word(ast, j, "}") || word(ast, j, ")") || word(ast, j, "]")) depth--;
            else if (!depth && word(ast, j, "tagged") && (word(ast, j - 1, "{") || word(ast, j - 1, ";"))) {
                size_t semi = j;
                while (semi < c && !word(ast, semi, ";")) semi++;
                if (semi >= c || semi < j + 4 || !same_text(k_em_tok(ast, semi - 1), fld)) continue;
                size_t name = semi - 1;
                if (!ident(ast, j + 1) || !(word_like(ast, name - 1) || word(ast, name - 1, "*"))) return false;
                /* the set, as the type of the module's own tags */
                KToken setk = k_em_tok(ast, j + 1);
                if (!ast->symbols) return false;
                const KSymbol *sym = k_symbol_resolve(ast->symbols, (keel_slice_char){ setk.len, (char *)setk.ptr });
                if (!sym || sym->kind != K_SYM_TAGS) return false;
                m->set_len = k_mangle_symbol(sym, (keel_slice_char){ sizeof m->set, m->set });
                if (m->set_len == 0 || m->set_len > sizeof m->set) return false;
                bool fptr = word(ast, name - 1, "*");
                const char *label = fptr ? "->tag" : ".tag";
                size_t q = 0;
                if (var.len + op.len + fld.len + 6 > sizeof m->disc) return false;
                memcpy(m->disc + q, var.ptr, var.len); q += var.len;
                memcpy(m->disc + q, op.ptr, op.len); q += op.len;
                memcpy(m->disc + q, fld.ptr, fld.len); q += fld.len;
                memcpy(m->disc + q, label, strlen(label)); q += strlen(label);
                m->disc_len = q;
                return true;
            }
        }
        return false;
    }
    return false;
}

/* Whether the `break` at token `target` of the arm (between open and close) is the arm's own:
 * BREAK_ARM when no loop, `switch`, `foreach` or `match` of the arm encloses it, BREAK_OWN when
 * one does, BREAK_DEEP when the nesting is past BLOCK_CAP. The walk keeps the open braces of
 * the arm, and for each one whether it is a construct's body; a construct without braces
 * (`for (...) r += i;`) owns the statement up to its `;`, an `else` included. */
static int break_owner(const KAst *ast, size_t open, size_t close, size_t target) {
    bool owned[BLOCK_CAP];
    size_t nb = 0, paren = 0;
    int hdr = 0;                    /* 1: a construct's header is open; 2: its body starts next */
    size_t hdr_paren = 0;
    bool unb = false;               /* inside an unbraced body */
    size_t unb_depth = 0, unb_paren = 0;
    for (size_t j = open + 1; j < close; j++) {
        if (j == target) {
            if (hdr == 2 || unb) return BREAK_OWN;
            for (size_t b = 0; b < nb; b++)
                if (owned[b]) return BREAK_OWN;
            return BREAK_ARM;
        }
        if (hdr == 1) {
            if (word(ast, j, "(")) paren++;
            else if (word(ast, j, ")") && --paren == hdr_paren) hdr = 2;
            continue;
        }
        if (hdr == 2) {
            hdr = 0;
            if (word(ast, j, "{")) {
                if (nb == BLOCK_CAP) return BREAK_DEEP;
                owned[nb++] = true;
                continue;
            }
            unb = true;                 /* the body is one statement */
            unb_depth = nb;
            unb_paren = paren;
        }
        if (word(ast, j, "for") || word(ast, j, "while") || word(ast, j, "switch") ||
            word(ast, j, "foreach") || word(ast, j, "match") || word(ast, j, "do")) {
            if (word(ast, j + 1, "(")) {
                hdr = 1;
                hdr_paren = paren;
            } else {
                hdr = 2;
            }
            continue;
        }
        if (word(ast, j, "{")) {
            if (nb == BLOCK_CAP) return BREAK_DEEP;
            owned[nb++] = false;
        } else if (word(ast, j, "}")) {
            if (nb == 0) return BREAK_DEEP;
            nb--;
            if (unb && nb == unb_depth && !word(ast, j + 1, "else")) unb = false;
        } else if (word(ast, j, "(")) {
            paren++;
        } else if (word(ast, j, ")")) {
            if (paren == 0) return BREAK_DEEP;
            paren--;
        } else if (word(ast, j, ";") && unb && nb == unb_depth && paren == unb_paren && !word(ast, j + 1, "else")) {
            unb = false;
        }
    }
    return BREAK_DEEP;
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
    if (!word(ast, m->kw, "match") || !word(ast, m->kw + 1, "(")) return false;
    m->operand = m->kw + 2;
    m->column = false;
    m->field = false;
    if (ident(ast, m->kw + 2) && word(ast, m->kw + 3, ")")) {
        m->rp = m->kw + 3;
    } else if (ident(ast, m->kw + 2) && (word(ast, m->kw + 3, "->") || word(ast, m->kw + 3, ".")) &&
               ident(ast, m->kw + 4) && word(ast, m->kw + 5, ")")) {
        m->rp = m->kw + 5;
        m->field = true;
    } else {
        /* a column of an extent holding a set (backend §5.6, rule 3) */
        size_t depth = 0, j = m->kw + 1;
        for (; j < ast->tokens.len; j++) {
            if (word(ast, j, "(")) depth++;
            else if (word(ast, j, ")") && --depth == 0) break;
        }
        if (j >= ast->tokens.len) return false;
        m->rp = j;
        for (size_t k = 0; k < ast->islands.len && !m->column; k++) {
            const KIsland *c = keel_buffer_KIsland_ptr(&ast->islands, k);
            size_t close;
            if (c->kind != K_ISLAND_COLUMN || c->anchor != m->operand) continue;
            if (!k_em_column_set(ast, c, m->set, sizeof m->set, &m->set_len, &close) || close + 1 != m->rp) return false;
            m->column = true;
        }
        if (!m->column) return false;
    }
    if (!word(ast, m->rp + 1, "{")) return false;
    m->open = m->rp + 1;
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
    if (m->column) goto labels;
    if (m->field) {
        if (!field_operand(ast, m)) return false;
        goto labels;
    }
    size_t dj;
    if (!decl_of(ast, m->kw, name, &dj)) return false;
    m->ptr = word(ast, dj - 1, "*");
    if (!resolve_operand(ast, is, m, name)) return false;

labels:
    /* the labels of the arms, at depth 1 of the match's braces */
    m->n = 0;
    depth = 0;
    for (size_t j = m->open + 1; j < m->close; j++) {
        if (word(ast, j, "{") || word(ast, j, "(") || word(ast, j, "[")) depth++;
        else if (word(ast, j, "}") || word(ast, j, ")") || word(ast, j, "]")) depth--;
        else if (!depth && ident(ast, j) && word(ast, j + 1, ":") &&
                 (word(ast, j - 1, "{") || word(ast, j - 1, ";") || word(ast, j - 1, "}") || word(ast, j - 1, ":"))) {
            if (m->n == MAX_LABELS) return false;
            /* a label has its line to itself, or its instruction shares the line (checked below) */
            if (!starts_line(ast, k_em_tok_off(ast, j))) return false;
            m->inl[m->n] = !ends_line(ast, k_em_tok_end(ast, j + 1));
            m->label[m->n] = j;
            m->colon[m->n++] = j + 1;
        }
    }
    if (m->n == 0) return false;

    /* an instruction on the label's line is the whole arm (backend §5.6, rule 6): the line ends
     * with its `;` or `}`, and the next token after the line is the next label or the match's `}` */
    for (size_t i = 0; i < m->n; i++) {
        if (!m->inl[i]) continue;
        size_t nl = line_end(ast, k_em_tok_end(ast, m->colon[i]));
        size_t p = nl;
        while (p > 0 && (ast->source.ptr[p - 1] == ' ' || ast->source.ptr[p - 1] == '\t')) p--;
        if (p == 0 || (ast->source.ptr[p - 1] != ';' && ast->source.ptr[p - 1] != '}')) return false;
        size_t lim = i + 1 < m->n ? m->label[i + 1] : m->close;
        size_t t = m->colon[i] + 1;
        while (t < lim && k_em_tok_off(ast, t) < nl) t++;
        if (t != lim) return false;
    }

    /* a `break` of the arm is a jump to the end, unless a construct of the arm owns it */
    for (size_t j = m->open + 1; j < m->close; j++)
        if (word(ast, j, "break") && break_owner(ast, m->open, m->close, j) == BREAK_DEEP) return false;
    return true;
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
            if (!m.column) q = put(w, q, m.disc, m.disc_len);
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
            if (m.column) {
                /* the operand stays where it is, for extent.c to lower: only `match (` and `) {` change */
                push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = kw_off, .skip = k_em_tok_end(ast, m.kw + 1) - kw_off,
                    .text = w, .text_len = sizeof "switch (" - 1 });
                push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = k_em_tok_off(ast, m.rp), .skip = k_em_tok_end(ast, m.open) - k_em_tok_off(ast, m.rp),
                    .text = w + sizeof "switch (" - 1, .text_len = q - (sizeof "switch (" - 1) });
            } else {
                push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = kw_off, .skip = k_em_tok_end(ast, m.open) - kw_off,
                    .text = w, .text_len = q });
            }
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
            if (starts_group && i > 0 && m.inl[i - 1]) {
                /* the previous arm closed on its own line: only its jump to the end comes here */
                q = put(w, q, "L", sizeof "L" - 1);
                q = put_uint(w, q, ln_of_a);
                q = put(w, q, "|", sizeof "|" - 1);
                q = put(w, q, ind, ind_n);
                q = put(w, q, "goto keel__m", sizeof "goto keel__m" - 1);
                q = put(w, q, id, idn);
                q = put(w, q, "_end;\nT|", sizeof "_end;\nT|" - 1);
            } else if (starts_group && i > 0) {
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
            /* an instruction on the label's line: the arm closes at the end of that line */
            if (m.inl[i])
                push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = line_end(ast, to), .skip = 0, .text = " }", .text_len = 2 });
        }

        /* the arm's `break` is the jump to the end, unless a construct of the arm owns it */
        for (size_t j = m.open + 1; j < m.close; j++) {
            if (!word(ast, j, "break")) continue;
            size_t off = k_em_tok_off(ast, j);
            if (off < a || off >= b) continue;
            if (break_owner(ast, m.open, m.close, j) != BREAK_ARM) continue;
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
            size_t q = 0;
            /* the last arm closed on its own line already: only the end label is left */
            if (!m.inl[m.n - 1]) {
                q = put(w, q, "}\n", sizeof "}\n" - 1);
                q = put(w, q, ind, ind_n);
            }
            q = put(w, q, "keel__m", sizeof "keel__m" - 1);
            q = put(w, q, id, idn);
            q = put(w, q, "_end: ;", sizeof "_end: ;" - 1);
            push_in_line(e, ed, cnt, a, b, (Edit){ .kind = EDIT_TEXT, .at = close_off, .skip = 1, .text = w, .text_len = q });
        }
    }
}
