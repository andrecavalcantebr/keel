/* Pass 3 (parser-design §3), stages 4a to 4h: type, name, call, ref and
 * implicit-init islands; array, array-index, index and range-index; defer,
 * foreach, walk and match; parallel and worker-exit; the else of a result;
 * extent and column — read from the token stream of each function and
 * variable declaration, signature and body alike, and of each extent.
 *
 * What resolves a call is the declared signature, never the call site (spec
 * §4.4): the callee module's own AST gives the parameter list, so the pass
 * needs the modules the file imports, and the file's own declarations for
 * unqualified calls. The only expression typing there is is the `container`
 * production's simplest form: an argument that is a single identifier the pass
 * saw declared with a keel type. Anything else stays unresolved and prints no
 * island, which is the "emit the call for C to validate" of spec §4.4. */
#include <stdint.h>
#include <string.h>
#include "engine/islands.h"
#include "engine/instances.h"
#include "engine/names.h"
#include "engine/parser/parser.h"
#include "engine/parser/parser_islands_internal.h"

KToken isl_tok(const KAst *a, size_t i) { return keel_buffer_KLexeme_ptr(&a->tokens, i)->token; }
bool isl_live(const KAst *a, size_t i) {
    return i < a->tokens.len && !keel_buffer_KLexeme_ptr(&a->tokens, i)->directive;
}
bool isl_punct(const KAst *a, size_t i, const char *s) { return isl_live(a, i) && k_token_is_punct(isl_tok(a, i), s); }
bool isl_word(const KAst *a, size_t i, const char *s) { return isl_live(a, i) && k_token_spelled(isl_tok(a, i), s); }
bool isl_ident(const KAst *a, size_t i) { return isl_live(a, i) && k_token_is_ident(isl_tok(a, i)); }

/* The first token whose spelling starts at or after `ptr`. */
size_t isl_index_at(const KAst *a, size_t from, const char *ptr) {
    size_t k = from;
    while (k < a->tokens.len && isl_tok(a, k).ptr < ptr) k++;
    return k;
}

KToken isl_enter(const KAst *a, size_t i, KLexer *lexer, TKPpKind *pp) {
    KToken t = isl_tok(a, i);
    k_lexer_init(lexer, (keel_slice_char){ (size_t)(a->source.ptr + a->source.len - t.ptr), t.ptr }, NULL);
    return k_lexer_next(lexer, pp);
}

/* the index of the ')' that closes the '(' at `open`; SIZE_MAX if none in [open, end) */
size_t isl_close_paren(const KAst *a, size_t open, size_t end) {
    size_t depth = 0;
    for (size_t j = open; j < end; j++) {
        if (isl_punct(a, j, "(")) depth++;
        else if (isl_punct(a, j, ")") && --depth == 0) return j;
    }
    return SIZE_MAX;
}

size_t isl_close_bracket(const KAst *a, size_t open, size_t end) {
    size_t depth = 0;
    for (size_t j = open; j < end; j++) {
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[") || isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}")) {
            if (--depth == 0) return isl_punct(a, j, "]") ? j : SIZE_MAX;
        }
    }
    return SIZE_MAX;
}

/* ---- the detail column ------------------------------------------------- */

void isl_put(Text *t, const char *p, size_t n) {
    if (n > sizeof t->s - t->n) { t->ok = false; return; }
    memcpy(t->s + t->n, p, n);
    t->n += n;
}
void isl_put_str(Text *t, const char *s) { isl_put(t, s, strlen(s)); }
void isl_put_uint(Text *t, size_t v) {
    char d[24]; size_t k = 0;
    do { d[k++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (k) isl_put(t, &d[--k], 1);
}
/* tokens as the dump prints them: one space between, none around `.`, `(`, `)`, `,` */
void isl_put_tokens(Text *t, const KAst *a, size_t first, size_t end) {
    for (size_t j = first; j < end; j++) {
        KToken k = isl_tok(a, j);
        if (j > first) {
            KToken p = isl_tok(a, j - 1);
            bool dot = k_token_is_punct(k, ".") || k_token_is_punct(p, ".");
            bool bracket = k_token_is_punct(k, "(") || k_token_is_punct(k, ")") ||
                           k_token_is_punct(k, ",") || k_token_is_punct(p, "(");
            if (!dot && !bracket) isl_put(t, " ", 1);
        }
        isl_put(t, k.ptr, k.len);
    }
}
void isl_put_glued(Text *t, const KAst *a, size_t first, size_t end) {
    for (size_t j = first; j < end; j++) isl_put(t, isl_tok(a, j).ptr, isl_tok(a, j).len);
}
/* source text, its white space collapsed to one space and trimmed at both ends */
void isl_put_source(Text *t, const char *p, const char *end) {
    bool space = false, any = false;
    for (; p < end; p++) {
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') { space = any; continue; }
        if (space) isl_put(t, " ", 1);
        isl_put(t, p, 1);
        space = false;
        any = true;
    }
}

/* islands stay sorted by anchor: a later declarator's island can be found
   before an earlier declarator's initializer is read */
/* A diagnostic argument built here must outlive the function that built it:
   the sink formats the message later. It is kept in the island text, after
   the details, where nothing reads it but the diagnostic. */
keel_slice_char isl_keep(Ctx *c, const Text *t) {
    KAst *a = c->ast;
    if (!t->ok || t->n > a->island_text.cap - a->island_text.len) return k_diag_text("its verbs");
    char *at = a->island_text.ptr + a->island_text.len;
    memcpy(at, t->s, t->n);
    a->island_text.len += t->n;
    return (keel_slice_char){ t->n, at };
}

bool isl_emit(Ctx *c, KIslandKind kind, size_t anchor, const Text *t) {
    KAst *a = c->ast;
    if (c->quiet) return true;
    if (!t->ok || a->islands.len == a->islands.cap || t->n > a->island_text.cap - a->island_text.len) {
        if (!c->failed) {
            KToken at = isl_tok(a, anchor);
            k_diag_emit(c->diag, K_DIAG_CAPACITY, at, (KDiagArgs){{at}});
            c->failed = true;
        }
        return false;
    }
    size_t pos = a->islands.len;
    while (pos > 0 && keel_buffer_KIsland_ptr(&a->islands, pos - 1)->anchor > anchor) pos--;
    memmove(a->islands.ptr + pos + 1, a->islands.ptr + pos, (a->islands.len - pos) * sizeof *a->islands.ptr);
    a->islands.len++;
    *keel_buffer_KIsland_ptr(&a->islands, pos) =
        (KIsland){ .kind = kind, .anchor = anchor, .text = a->island_text.len, .text_len = t->n };
    memcpy(a->island_text.ptr + a->island_text.len, t->s, t->n);
    a->island_text.len += t->n;
    return true;
}

/* ---- locals ------------------------------------------------------------ */

const Local *isl_find_local(const Ctx *c, keel_slice_char name) {
    for (size_t i = c->local_count; i-- > 0;)
        if (k_symtab_same_name(c->locals[i].name, name)) return &c->locals[i];
    return NULL;
}

/* The value of a decimal literal, or of a name whose initializer is one (spec
   §4.2, item 14): keel does not calculate expressions. */
bool isl_digits(keel_slice_char s, long *out) {
    while (s.len && (s.ptr[0] == ' ' || s.ptr[0] == '\t' || s.ptr[0] == '\n')) { s.ptr++; s.len--; }
    while (s.len && (s.ptr[s.len - 1] == ' ' || s.ptr[s.len - 1] == '\t' || s.ptr[s.len - 1] == '\n')) s.len--;
    if (!s.len || s.len > 9) return false;
    long v = 0;
    for (size_t i = 0; i < s.len; i++) {
        if (s.ptr[i] < '0' || s.ptr[i] > '9') return false;
        v = v * 10 + (s.ptr[i] - '0');
    }
    *out = v;
    return true;
}

bool isl_decimal_of_token(const Ctx *c, KToken t, long *out) {
    if (k_token_is_number(t)) return isl_digits(t, out);
    if (!k_token_is_ident(t)) return false;
    const Local *l = isl_find_local(c, t);
    if (l) return l->is_constexpr && isl_digits(l->value, out);
    const KSymbol *sym = k_symbol_resolve(c->ast->symbols, t);
    return sym && sym->kind == K_SYM_CONSTANT && isl_digits(sym->value, out);
}

bool isl_decimal_of(const Ctx *c, Range r, long *out) {
    return r.end == r.first + 1 && isl_decimal_of_token(c, isl_tok(c->ast, r.first), out);
}

/* The dimensions an `array` declares, in order, from the text `[2,3][4]`; -1 for
   one that is not a known decimal (a binder, an expression). */
size_t isl_dims_of(const Ctx *c, keel_slice_char text, long out[8]) {
    KLexer lexer;
    k_lexer_init(&lexer, text, NULL);
    TKPpKind pp;
    size_t n = 0, depth = 0, tokens = 0;
    KToken only = {0};
    for (KToken t = k_lexer_next(&lexer, &pp); t.len; t = k_lexer_next(&lexer, &pp)) {
        bool open = k_token_is_punct(t, "["), close = k_token_is_punct(t, "]");
        if (open && depth++ == 0) { tokens = 0; continue; }
        if (close && --depth == 0) {
            long v;
            if (n < 8) out[n++] = tokens == 1 && isl_decimal_of_token(c, only, &v) ? v : -1;
            continue;
        }
        if (depth == 1 && k_token_is_punct(t, ",")) {
            long v;
            if (n < 8) out[n++] = tokens == 1 && isl_decimal_of_token(c, only, &v) ? v : -1;
            tokens = 0;
            continue;
        }
        if (depth >= 1) { tokens++; only = t; }
    }
    return n;
}

void isl_add_local(Ctx *c, Local local) {
    if (c->local_count == K_LOCALS_MAX) {
        if (!c->failed) {
            k_diag_emit(c->diag, K_DIAG_CAPACITY, local.name, (KDiagArgs){{local.name}});
            c->failed = true;
        }
        return;
    }
    local.depth = c->depth;
    /* a binder a construction produces depends on its container (spec §4.12, rule 15) */
    for (size_t k = 0; k < c->pending_count; k++)
        if (k_symtab_same_name(c->pending[k].name, local.name) && local.decl_at < c->pending[k].until &&
            c->pending[k].parent >= 0 && (size_t)c->pending[k].parent < c->local_count) {
            local.deps[0] = c->pending[k].parent;
            local.ndeps = 1;
            local.local_origin = c->locals[c->pending[k].parent].local_origin;
        }
    c->locals[c->local_count++] = local;
}

/* The dimensions a `[...]` part declares: one per comma inside a group, one
   per group — `[2,3,4]` and `[2][3][4]` are both rank 3. */
int isl_rank_of(const char *p, const char *end) {
    int rank = 0, depth = 0;
    for (; p < end; p++) {
        if (*p == '[' || *p == '(' || *p == '{') { if (depth++ == 0 && *p == '[') rank++; }
        else if (*p == ']' || *p == ')' || *p == '}') depth--;
        else if (*p == ',' && depth == 1) rank++;
    }
    return rank;
}

/* ---- the module a call lands in ---------------------------------------- */

const KAstNode *isl_node_at(const KAst *a, size_t i) { return keel_buffer_KAstNode_ptr(&a->nodes, i); }

const KAstNode *isl_module_node(const KAst *a) { return isl_node_at(a, a->module); }

bool isl_generic(const KAst *a) {
    const KAstNode *m = isl_module_node(a);
    return m->dim_first != m->dim_end || m->tags_first != m->tags_end || m->type_first != m->type_end;
}

/* `keel.arena` → keel_arena */
void isl_put_module_prefix(Text *t, const KAst *a) {
    const KAstNode *m = isl_module_node(a);
    for (size_t j = m->name_first; j < m->name_end; j++) {
        if (isl_punct(a, j, ".")) isl_put(t, "_", 1);
        else isl_put(t, isl_tok(a, j).ptr, isl_tok(a, j).len);
    }
}

bool isl_module_named(const KAst *a, const char *name) {
    const KAstNode *m = isl_module_node(a);
    char buf[64]; size_t n = 0;
    for (size_t j = m->name_first; j < m->name_end; j++) {
        KToken k = isl_tok(a, j);
        if (k.len > sizeof buf - n) return false;
        memcpy(buf + n, k.ptr, k.len); n += k.len;
    }
    return n == strlen(name) && memcmp(buf, name, n) == 0;
}

/* the base owns the keel_/KEEL_ names (spec §2.5): module `keel` and every
   `keel.<name>`, that is, a module whose first component is `keel` (`keelx`
   is not) */
bool isl_module_in_base(const KAst *a) {
    const KAstNode *m = isl_module_node(a);
    return m->kind == K_AST_MODULE && m->name_first < m->name_end &&
           k_token_spelled(isl_tok(a, m->name_first), "keel");
}

bool isl_binder(const KAst *a, keel_slice_char name) {
    const KAstNode *m = isl_module_node(a);
    for (size_t j = m->type_first; j < m->type_end; j++)
        if (isl_ident(a, j) && k_symtab_same_name(isl_tok(a, j), name)) return true;
    return false;
}

keel_slice_char isl_modifier_of(const KAst *a) {
    for (size_t i = 0; i < a->nodes.len; i++)
        if (isl_node_at(a, i)->kind == K_AST_MODIFIER)
            return isl_tok(a, isl_node_at(a, i)->name_first);
    return (keel_slice_char){0};
}

/* The parameters of the function node `fn`, in `a`. SIZE_MAX if they do not fit. */
size_t isl_params_of(const KAst *a, const KAstNode *fn, Param *out, size_t cap) {
    size_t open = fn->name_end, n = 0, depth = 0, start = open + 1;
    if (!isl_punct(a, open, "(")) return 0;
    for (size_t j = open + 1; j < a->tokens.len; j++) {
        bool closing = false;
        if (isl_punct(a, j, "(") || isl_punct(a, j, "[")) depth++;
        else if (isl_punct(a, j, "]") || isl_punct(a, j, ")")) {
            if (depth) depth--;
            else if (isl_punct(a, j, ")")) closing = true;
        }
        if (!closing && !(depth == 0 && isl_punct(a, j, ","))) continue;
        if (j > start && !(j == start + 1 && isl_word(a, start, "void"))) {
            if (n == cap) return SIZE_MAX;
            /* a role leads the parameter, and is not its type (spec §4.12) */
            KRole role = k_token_role(isl_tok(a, start));
            size_t lead = role != K_ROLE_NONE ? start + 1 : start;
            Param p = { .is_type = isl_word(a, lead, "type"), .is_array = isl_word(a, lead, "array"),
                        .role = role, .first = start, .end = j };
            for (size_t k = lead; k < j; k++) {
                if (isl_punct(a, k, "*") || (isl_punct(a, k, "[") && !p.is_array)) p.pointer = true;
                if (!p.base.len && isl_ident(a, k) && !((p.is_type || p.is_array) && k == lead)) p.base = isl_tok(a, k);
            }
            if (p.base.len && a->symbols) {
                const KSymbol *s = k_symtab_lookup(a->symbols, p.base);
                p.is_protocol = s && s->kind == K_SYM_PROTOCOL;
            }
            out[n++] = p;
        }
        start = j + 1;
        if (closing) break;
    }
    return n;
}


/* The declaration of `verb` in `a` with `arity` parameters (any arity when
   `arity` is SIZE_MAX), and whether the name is declared in several. */
Sig isl_find_verb(const KAst *a, keel_slice_char verb, size_t arity) {
    Sig sig = {0};
    uint64_t seen = 0;
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *fn = isl_node_at(a, i);
        if (fn->kind != K_AST_FUNCTION || !k_symtab_same_name(isl_tok(a, fn->name_first), verb)) continue;
        Param p[K_PARAMS_MAX];
        size_t n = isl_params_of(a, fn, p, K_PARAMS_MAX);
        if (n == SIZE_MAX) continue;
        seen |= (uint64_t)1 << n;
        if (!sig.found && (arity == SIZE_MAX || n == arity)) {
            memcpy(sig.p, p, n * sizeof *p);
            sig.n = n;
            sig.found = true;
            sig.fn = fn;
            keel_slice_char own = isl_modifier_of(a);
            for (size_t j = fn->first; own.len && j < fn->name_first; j++)
                if (isl_ident(a, j) && k_symtab_same_name(isl_tok(a, j), own)) sig.producer = true;
        }
    }
    sig.overloaded = (seen & (seen - 1)) != 0;
    sig.rank_binder = -1;
    /* the vector accessor: a second parameter `size_t idx[static N]`, N a `dim` binder */
    if (sig.found && sig.n == 2) {
        const Param *p = &sig.p[1];
        const KAstNode *m = isl_module_node(a);
        for (size_t k = p->first; k + 3 < p->end; k++) {
            if (!isl_punct(a, k, "[") || !isl_word(a, k + 1, "static") || !isl_punct(a, k + 3, "]")) continue;
            int at = 0;
            for (size_t j = m->dim_first; j < m->dim_end; j++) {
                if (!isl_ident(a, j) || isl_word(a, j, "dim")) continue;
                if (k_symtab_same_name(isl_tok(a, j), isl_tok(a, k + 2))) { sig.rank_binder = at; break; }
                at++;
            }
        }
    }
    return sig;
}

bool isl_put_verb_suffix(const Ctx *c, const KType *k, const Sig *sig, size_t arity, Text *t) {
    if (sig->rank_binder < 0) {
        if (sig->overloaded && arity > 1) isl_put_uint(t, arity - 1);
        return true;
    }
    /* the instance's dimensions are in its symbol, after the modifier and before
       the type arguments, as decimals: `grids_grid_3_f32` (backend §2.2) */
    if (k->kind != KT_MODIFIER || !*k->symbol || !k->module) return false;
    const KAst *home = isl_ast_of(c, k->module);
    if (!home) return false;
    const KAstNode *m = isl_module_node(home);
    int dims = 0;
    for (size_t j = m->dim_first; j < m->dim_end; j++)
        if (isl_ident(home, j) && !isl_word(home, j, "dim")) dims++;
    const char *s = k->symbol;
    size_t hi = strlen(s);
    for (int g = k->argc; g-- > 0; ) {
        const char *as = isl_kt_symbol(c, k->arg[g]);
        size_t al = strlen(as);
        if (!al || hi < al + 1 || memcmp(s + hi - al, as, al) != 0 || s[hi - al - 1] != '_') return false;
        hi -= al + 1;
    }
    /* the last `dims` groups are the dimensions; the one wanted is rank_binder */
    size_t end[8], start[8];
    int n = 0;
    while (n < dims && n < 8) {
        size_t e = hi, b = hi;
        while (b > 0 && s[b - 1] >= '0' && s[b - 1] <= '9') b--;
        if (b == e || b == 0 || s[b - 1] != '_') return false;
        start[n] = b; end[n] = e; n++;
        hi = b - 1;
    }
    if (n != dims || sig->rank_binder >= dims) return false;
    int g = dims - 1 - sig->rank_binder;      /* collected from the end */
    isl_put_str(t, "_");
    isl_put(t, s + start[g], end[g] - start[g]);
    return true;
}

const KModule *isl_module_alias(const KSymbolTable *symbols, keel_slice_char name) {
    for (size_t i = 0; i < symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin && k_symtab_same_name(s->name, name)) return s->origin;
    }
    return NULL;
}

/* the module a file imports under its own declared name, whatever the alias */
const KModule *isl_module_by_name(const KSymbolTable *symbols, const char *name) {
    for (size_t i = 0; i < symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin && s->origin->name.len == strlen(name) &&
            !memcmp(s->origin->name.ptr, name, s->origin->name.len))
            return s->origin;
    }
    return NULL;
}

/* ---- protocols ---------------------------------------------------------- */

/* The declaration of protocol `name` as `home` sees it — its own, or the one of
   the module its symbol table says the name comes from — with the verbs of its
   components folded in (spec §5.1, rule 7). */
bool isl_protocol_decl(const Ctx *c, const KAst *home, keel_slice_char name, KProtocolDecl *out, int depth) {
    if (depth > 4) return false;
    const KAst *where = home;
    const KSymbol *s = home->symbols ? k_symtab_lookup(home->symbols, name) : NULL;
    if (s && s->origin && s->origin->ast) where = s->origin->ast;
    for (size_t i = 0; i < where->nodes.len; i++) {
        const KAstNode *n = isl_node_at(where, i);
        if (n->kind != K_AST_PROTOCOL || !k_symtab_same_name(isl_tok(where, n->name_first), name)) continue;
        size_t kw = n->first;
        while (kw < n->name_first && !isl_word(where, kw, "protocol")) kw++;
        KToken t = isl_tok(where, kw);
        KLexer lexer;
        TKPpKind pp;
        k_lexer_init(&lexer, (keel_slice_char){ (size_t)(where->source.ptr + where->source.len - t.ptr), (char *)t.ptr }, NULL);
        KToken first = k_lexer_next(&lexer, &pp), next;
        KSymbol scratch[2];
        KSymbolTable none;
        k_symtab_init(&none, scratch, 2);
        if (!k_scan_decl_protocol(&lexer, first, &none, out, &next, &pp)) return false;
        for (size_t k = 0; k < out->component_count; k++) {
            KProtocolDecl part;
            if (!isl_protocol_decl(c, where, out->components[k], &part, depth + 1)) return false;
            for (size_t j = 0; j < part.verb_count && out->verb_count < K_PROTOCOL_MAX_ITEMS; j++)
                out->verbs[out->verb_count++] = part.verbs[j];
            for (size_t j = 0; j < part.assoc_count && out->assoc_count < K_PROTOCOL_MAX_ITEMS; j++)
                out->assoc[out->assoc_count++] = part.assoc[j];
        }
        return true;
    }
    return false;
}

Sig isl_find_verb(const KAst *a, keel_slice_char verb, size_t arity);
void isl_diag3(Ctx *c, KDiagId id, keel_slice_char at, keel_slice_char x, keel_slice_char y,
                  keel_slice_char z);

/* Whether the module `own` declares every verb of `pd` (spec §5.1, rule 3),
   naming the ones it lacks in `missing`; a verb whose roles differ from the
   prototype's is protocol-role-mismatch (rule 10). */
/* the last name of the type a function returns, without its stars */
static keel_slice_char return_name(const KAst *home, const KAstNode *fn) {
    size_t e = fn->name_first;
    while (e > fn->first && isl_punct(home, e - 1, "*")) e--;
    return e > fn->first && (isl_ident(home, e - 1) || k_token_is_c_word(isl_tok(home, e - 1))) ? isl_tok(home, e - 1)
                                                                                       : (keel_slice_char){0};
}

bool isl_satisfies(Ctx *c, const KAst *own, keel_slice_char receiver, const KProtocolDecl *pd, Text *missing, KToken at) {
    bool ok = true;
    for (size_t i = 0; i < pd->verb_count; i++) {
        const KProtocolVerb *v = &pd->verbs[i];
        Sig sig = isl_find_verb(own, v->name, v->arity);
        /* the verb must take this type as its receiver, and not another of the module's */
        if (sig.found && receiver.len && v->receiver < sig.n && !sig.p[v->receiver].is_array &&
            !k_symtab_same_name(sig.p[v->receiver].base, receiver))
            sig.found = false;
        if (!sig.found) {
            if (missing->n) isl_put_str(missing, ", ");
            isl_put(missing, v->name.ptr, v->name.len);
            ok = false;
            continue;
        }
        bool written = false, differ = false;
        for (size_t k = 0; k < sig.n && k < K_PROTOCOL_MAX_PARAMS; k++) {
            if (sig.p[k].role != K_ROLE_NONE) written = true;
            if (sig.p[k].role != v->roles[k]) differ = true;
        }
        if (written && differ)
            isl_diag3(c, K_DIAG_PROTOCOL_ROLE_MISMATCH, at, v->name, pd->name, (keel_slice_char){0});
    }
    /* an associated type binds once: every required verb that returns it
       returns the same type in the implementer (spec §5.1, rule 7) */
    for (size_t t = 0; ok && t < pd->assoc_count; t++) {
        bool again = false;                     /* the same name from two components */
        for (size_t u = 0; u < t; u++) if (k_symtab_same_name(pd->assoc[u], pd->assoc[t])) again = true;
        if (again) continue;
        keel_slice_char bound = {0};
        for (size_t i = 0; i < pd->verb_count; i++) {
            const KProtocolVerb *v = &pd->verbs[i];
            if (!k_symtab_same_name(v->returns, pd->assoc[t])) continue;
            Sig sig = isl_find_verb(own, v->name, v->arity);
            if (!sig.found) continue;
            keel_slice_char r = return_name(own, sig.fn);
            if (!r.len) continue;
            if (!bound.len) bound = r;
            else if (!k_symtab_same_name(bound, r)) {
                isl_diag3(c, K_DIAG_ASSOCIATED_TYPE_CONFLICT, at, pd->assoc[t], bound, r);
                break;
            }
        }
    }
    return ok;
}

/* The associated type the function `fn` returns, written `C.name`; empty when
   its return type is not one. */
static keel_slice_char returned_associated(const KAst *home, const KAstNode *fn) {
    for (size_t j = fn->first; j + 2 < fn->name_first; j++)
        if (isl_ident(home, j) && isl_punct(home, j + 1, ".") && isl_ident(home, j + 2)) return isl_tok(home, j + 2);
    return (keel_slice_char){0};
}

bool isl_returns_associated(const KAst *home, const KAstNode *fn) {
    return returned_associated(home, fn).len != 0;
}

/* The verb of `pd` the call is resolved as: the one whose result is the
   associated type the function returns, or else the first that takes the
   receiver. */
const KProtocolVerb *isl_verb_for_return(const KAst *home, const KAstNode *fn, const KProtocolDecl *pd) {
    keel_slice_char assoc = returned_associated(home, fn);
    for (size_t i = 0; assoc.len && i < pd->verb_count; i++)
        if (k_symtab_same_name(pd->verbs[i].returns, assoc)) return &pd->verbs[i];
    for (size_t i = 0; i < pd->verb_count; i++)
        if (pd->verbs[i].receiver == 0) return &pd->verbs[i];
    return NULL;
}

/* ---- calls ------------------------------------------------------------- */

keel_slice_char isl_range_text(const KAst *a, Range r) {
    if (r.first >= r.end) return (keel_slice_char){0};
    KToken f = isl_tok(a, r.first), l = isl_tok(a, r.end - 1);
    return (keel_slice_char){ (size_t)(l.ptr + l.len - f.ptr), f.ptr };
}

void isl_diag3(Ctx *c, KDiagId id, keel_slice_char at, keel_slice_char x, keel_slice_char y,
                  keel_slice_char z) {
    if (c->quiet) return;
    k_diag_emit(c->diag, id, at, (KDiagArgs){{ x, y, z }});
}

/* The shortest name a module is imported under: the alias, when there is one. */
keel_slice_char isl_alias_of(const KSymbolTable *symbols, const KModule *m) {
    keel_slice_char best = m->name;
    for (size_t i = 0; i < symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin == m && s->name.len < best.len) best = s->name;
    }
    return best;
}

/* the first token of a piece of source text */
KToken isl_enter_text(keel_slice_char text, KLexer *lexer, TKPpKind *pp) {
    k_lexer_init(lexer, text, NULL);
    return k_lexer_next(lexer, pp);
}


/* What the `container` production (spec §2.2) makes of an argument: one
   identifier, a shape it admits but this pass cannot type (a field, an index,
   a verb over a container), or C — a literal, an operator, a cast, a call of
   anything but a qualified verb — which gives no container identity (§4.4). */
Shape isl_container_shape(const KAst *a, Range r) {
    if (r.end == r.first + 1 && isl_ident(a, r.first)) return SHAPE_NAME;
    size_t depth = 0;
    for (size_t j = r.first; j < r.end; j++) {
        if (isl_punct(a, j, "(")) {
            if (depth == 0) {
                size_t k = isl_close_paren(a, j, r.end);
                if (k != SIZE_MAX && k + 1 < r.end &&
                    (isl_ident(a, k + 1) || k_token_is_number(isl_tok(a, k + 1)))) return SHAPE_C;   /* a cast */
                bool verb = j >= r.first + 3 && isl_ident(a, j - 1) && isl_punct(a, j - 2, ".") && isl_ident(a, j - 3);
                if (j > r.first && isl_ident(a, j - 1) && !verb) return SHAPE_C;                  /* a C call */
            }
            depth++;
        } else if (isl_punct(a, j, "[") || isl_punct(a, j, "{")) depth++;
        else if (isl_punct(a, j, ")") || isl_punct(a, j, "]") || isl_punct(a, j, "}")) { if (depth) depth--; }
        else if (depth == 0) {
            KToken t = isl_tok(a, j);
            if (k_token_is_number(t) || k_token_is_string(t) || k_token_is_char(t)) return SHAPE_C;
            bool prefix = j == r.first && (isl_punct(a, j, "*") || isl_punct(a, j, "&"));
            if (!(isl_ident(a, j) || isl_punct(a, j, ".") || isl_punct(a, j, "->") || prefix)) return SHAPE_C;
        }
    }
    return SHAPE_CONTAINER;
}

const Local *isl_arg_local(const Ctx *c, Range r) {
    return r.end == r.first + 1 && isl_ident(c->ast, r.first) ? isl_find_local(c, isl_tok(c->ast, r.first)) : NULL;
}

void isl_record_seed(Ctx *c, int t, const char *symbol, KClosureKind kind, const KModule *module);
keel_slice_char isl_alias_of(const KSymbolTable *symbols, const KModule *m);
/* ---- seeds from calls ---------------------------------------------------- */

/* The instances the calls of the module take without writing them: a verb of
   `keel.array` over an element type, and a function over a protocol for a
   concrete type (spec §4.4, §4.14). Kept as flat type trees, outside the
   pool, until the closure (one translation at a time). */
#define K_SEED_MAX 48
static KType seed_nodes[K_SEED_MAX * 6];
static size_t seed_node_count;
static struct { int root; KClosureKind kind; char symbol[K_SYMBOL_MAX]; const KModule *module; } seeds[K_SEED_MAX];
static size_t seed_count;

static int seed_copy(Ctx *c, int t) {
    if (t < 0 || seed_node_count == sizeof seed_nodes / sizeof *seed_nodes) return -1;
    int at = (int)seed_node_count++;
    seed_nodes[at] = c->types[t];
    if (c->types[t].kind == KT_MODIFIER)
        for (int i = 0; i < c->types[t].argc; i++) seed_nodes[at].arg[i] = seed_copy(c, c->types[t].arg[i]);
    if (c->types[t].kind == KT_ARRAY) seed_nodes[at].elem = seed_copy(c, c->types[t].elem);
    return at;
}

void isl_record_seed(Ctx *c, int t, const char *symbol, KClosureKind kind, const KModule *module) {
    if (c->quiet || !*symbol) return;
    for (size_t k = 0; k < seed_count; k++) if (!strcmp(seeds[k].symbol, symbol)) return;
    if (seed_count == K_SEED_MAX) return;
    int root = kind == K_CLOSURE_INSTANCE ? seed_copy(c, t) : -1;
    if (kind == K_CLOSURE_INSTANCE && root < 0) return;
    seeds[seed_count].root = root;
    seeds[seed_count].kind = kind;
    seeds[seed_count].module = module;
    strncpy(seeds[seed_count].symbol, symbol, K_SYMBOL_MAX - 1);
    seeds[seed_count].symbol[K_SYMBOL_MAX - 1] = 0;
    seed_count++;
}

/* a string into the closure's pool */
static bool pool(Ctx *c, const char *p, size_t n, size_t *at) {
    KAst *a = c->ast;
    if (n > a->closure_text.cap - a->closure_text.len) {
        if (!c->failed) {
            KToken t = isl_tok(a, a->module);
            k_diag_emit(c->diag, K_DIAG_CAPACITY, t, (KDiagArgs){{ t }});
            c->failed = true;
        }
        return false;
    }
    *at = a->closure_text.len;
    memcpy(a->closure_text.ptr + a->closure_text.len, p, n);
    a->closure_text.len += n;
    return true;
}

/* Parser §5, codegen §7.3: an instance comes out whole, so the instances its
   available verbs mention come with it, until nothing new appears. The seeds
   are the instances the module writes (`inst`) and the ones its calls take;
   the result is KAst.closure, with the verbs each instance loses in
   KAst.unavailable (cgen-tool §5.2 prints them). */
static void close_instances(Ctx *c) {
    KAst *a = c->ast;
    c->type_count = 0;
    c->nest = 1;                            /* no resolution resets the pool under the closure */
    int inst[K_CLOSURE_MAX];
    int from[K_CLOSURE_MAX];
    KClosureOrigin origin[K_CLOSURE_MAX];
    keel_slice_char verb[K_CLOSURE_MAX];
    size_t arity[K_CLOSURE_MAX];
    size_t count = 0;
    for (size_t i = 0; i < a->instances.len && count < K_CLOSURE_MAX; i++) {
        const KInstanceUse *u = keel_buffer_KInstanceUse_ptr(&a->instances, i);
        int t = isl_kt_from_text(c, a->symbols, isl_range_text(a, (Range){ u->first, u->end }));
        if (t < 0 || isl_kt(c, t)->kind != KT_MODIFIER || !*isl_kt(c, t)->symbol) continue;
        inst[count] = t;
        origin[count] = K_CLOSURE_WRITTEN;
        from[count++] = -1;
    }
    for (size_t k = 0; k < seed_count && count < K_CLOSURE_MAX; k++) {
        if (seeds[k].kind != K_CLOSURE_INSTANCE) continue;
        bool known = false;
        for (size_t e = 0; e < count; e++) if (!strcmp(isl_kt(c, inst[e])->symbol, seeds[k].symbol)) known = true;
        if (known) continue;
        int t = isl_kt_deep(c, seed_nodes, seeds[k].root);
        if (t < 0) break;
        isl_kt_set_symbol(isl_kt(c, t), seeds[k].symbol, strlen(seeds[k].symbol));
        inst[count] = t;
        origin[count] = K_CLOSURE_CALL;
        from[count++] = -1;
    }
    for (size_t q = 0; q < count && !c->failed; q++) {
        const KType *k = isl_kt(c, inst[q]);
        const KAst *home = k->module ? isl_ast_of(c, k->module) : NULL;
        if (!home || home == a) continue;
        for (size_t i = 0; i < home->nodes.len; i++) {
            const KAstNode *fn = isl_node_at(home, i);
            if (fn->kind != K_AST_FUNCTION) continue;
            Param p[K_PARAMS_MAX];
            size_t np = isl_params_of(home, fn, p, K_PARAMS_MAX);
            if (np == SIZE_MAX || (np && p[0].is_protocol)) continue;     /* of the module, not of an instance */
            const char *cause = NULL;
            if (!isl_verb_available(c, home, fn, inst[q], &cause)) continue;
            int pinned = c->type_count;
            int found[16];
            size_t nf = isl_instances_in(c, isl_ret_type(c, home, fn, inst[q]), found, 0, 16, 0);
            for (size_t m = 0; m < np; m++) {
                if (p[m].is_type || p[m].is_array) continue;
                Range r = isl_param_type(home, &p[m]);
                if (r.end > r.first) nf = isl_instances_in(c, isl_kt_from_home(c, home, isl_range_text(home, r), inst[q]), found, nf, 16, 0);
            }
            KType scratch[64];
            int roots[16];
            size_t ns = 0, nr = 0;
            for (size_t f = 0; f < nf; f++) {
                bool known = false;
                for (size_t e = 0; e < count; e++) if (!strcmp(isl_kt(c, inst[e])->symbol, isl_kt(c, found[f])->symbol)) known = true;
                for (size_t e = 0; e < nr; e++) if (!strcmp(scratch[roots[e]].symbol, isl_kt(c, found[f])->symbol)) known = true;
                if (known || count + nr >= K_CLOSURE_MAX) continue;
                /* flatten the tree into scratch, re-indexed */
                int stack[64], map[64];
                size_t sn = 0, base = ns;
                stack[sn++] = found[f];
                size_t visited = 0;
                int order[64];
                while (sn && ns < 64) {
                    int x = stack[--sn];
                    order[visited] = x;
                    map[visited++] = (int)ns;
                    scratch[ns++] = c->types[x];
                    if (c->types[x].kind == KT_MODIFIER)
                        for (int g = c->types[x].argc; g-- > 0; ) if (c->types[x].arg[g] >= 0 && sn < 64) stack[sn++] = c->types[x].arg[g];
                }
                for (size_t v = base; v < ns; v++)
                    if (scratch[v].kind == KT_MODIFIER)
                        for (int g = 0; g < scratch[v].argc; g++)
                            for (size_t o = 0; o < visited; o++)
                                if (order[o] == scratch[v].arg[g]) { scratch[v].arg[g] = map[o]; break; }
                roots[nr++] = (int)base;
            }
            c->type_count = pinned;
            for (size_t e = 0; e < nr; e++) {
                int t = isl_kt_deep(c, scratch, roots[e]);
                if (t < 0) break;
                origin[count] = K_CLOSURE_VERB;
                from[count] = (int)q;
                verb[count] = isl_tok(home, fn->name_first);
                arity[count] = np;
                inst[count++] = t;
            }
        }
    }
    /* the entries: the written ones in their order, then the others by symbol */
    size_t order[K_CLOSURE_MAX], n = 0, written = 0;
    for (size_t q = 0; q < count; q++) if (origin[q] == K_CLOSURE_WRITTEN) order[n++] = q;
    written = n;
    for (size_t q = 0; q < count; q++) if (origin[q] != K_CLOSURE_WRITTEN) order[n++] = q;
    for (size_t x = written + 1; x < n; x++)
        for (size_t y = x; y > written && strcmp(isl_kt(c, inst[order[y - 1]])->symbol, isl_kt(c, inst[order[y]])->symbol) > 0; y--) {
            size_t tmp = order[y]; order[y] = order[y - 1]; order[y - 1] = tmp;
        }
    int slot[K_CLOSURE_MAX];
    for (size_t x = 0; x < n; x++) slot[order[x]] = (int)x;
    a->closure.len = 0;
    a->unavailable.len = 0;
    for (size_t x = 0; x < n && !c->failed; x++) {
        size_t q = order[x];
        const KType *k = isl_kt(c, inst[q]);
        if (a->closure.len == a->closure.cap) {
            KToken t = isl_tok(a, a->module);
            k_diag_emit(c->diag, K_DIAG_CAPACITY, t, (KDiagArgs){{ t }});
            c->failed = true;
            break;
        }
        KClosure e = { .kind = K_CLOSURE_INSTANCE, .module = k->module, .modifier = k->name, .origin = origin[q],
                       .from = from[q] >= 0 ? slot[from[q]] : -1, .verb = origin[q] == K_CLOSURE_VERB ? verb[q] : (keel_slice_char){0},
                       .arity = origin[q] == K_CLOSURE_VERB ? arity[q] : 0 };
        e.symbol_len = strlen(k->symbol);
        memcpy(e.symbol, k->symbol, e.symbol_len);
        for (int g = 0; g < k->argc && g < K_CLOSURE_ARGS; g++) {
            const char *sym = isl_kt_symbol(c, k->arg[g]);
            e.arg_instance[e.argc] = -1;
            for (size_t o = 0; o < count; o++) if (!strcmp(isl_kt(c, inst[o])->symbol, sym) && *sym) e.arg_instance[e.argc] = slot[o];
            if (!pool(c, sym, strlen(sym), &e.arg_text[e.argc])) break;
            e.arg_len[e.argc++] = strlen(sym);
        }
        /* the verbs the instance loses */
        e.unavailable_first = a->unavailable.len;
        const KAst *home = k->module ? isl_ast_of(c, k->module) : NULL;
        for (size_t i = 0; home && home != a && i < home->nodes.len; i++) {
            const KAstNode *fn = isl_node_at(home, i);
            if (fn->kind != K_AST_FUNCTION) continue;
            Param p[K_PARAMS_MAX];
            size_t np = isl_params_of(home, fn, p, K_PARAMS_MAX);
            if (np == SIZE_MAX || (np && p[0].is_protocol)) continue;
            const char *cause = NULL;
            if (isl_verb_available(c, home, fn, inst[q], &cause)) continue;
            if (a->unavailable.len == a->unavailable.cap) break;
            KUnavailable u = { .verb = isl_tok(home, fn->name_first), .arity = np };
            if (!pool(c, cause ? cause : "", cause ? strlen(cause) : 0, &u.cause_text)) break;
            u.cause_len = cause ? strlen(cause) : 0;
            *keel_buffer_KUnavailable_ptr(&a->unavailable, a->unavailable.len++) = u;
        }
        e.unavailable_count = a->unavailable.len - e.unavailable_first;
        *keel_buffer_KClosure_ptr(&a->closure, a->closure.len++) = e;
    }
    /* the functions over a protocol the calls take, one per concrete type,
       by symbol (codegen §9) */
    size_t fns[K_SEED_MAX], nfn = 0;
    for (size_t k = 0; k < seed_count; k++) if (seeds[k].kind == K_CLOSURE_PROTOCOL_FUNCTION) fns[nfn++] = k;
    for (size_t x = 1; x < nfn; x++)
        for (size_t y = x; y > 0 && strcmp(seeds[fns[y - 1]].symbol, seeds[fns[y]].symbol) > 0; y--) {
            size_t tmp = fns[y]; fns[y] = fns[y - 1]; fns[y - 1] = tmp;
        }
    for (size_t f = 0; f < nfn && !c->failed; f++) {
        size_t k = fns[f];
        if (a->closure.len == a->closure.cap) continue;
        KClosure e = { .kind = K_CLOSURE_PROTOCOL_FUNCTION, .module = seeds[k].module, .origin = K_CLOSURE_CALL, .from = -1 };
        e.symbol_len = strlen(seeds[k].symbol);
        memcpy(e.symbol, seeds[k].symbol, e.symbol_len);
        *keel_buffer_KClosure_ptr(&a->closure, a->closure.len++) = e;
    }
    c->nest = 0;
}

/* ---- extern_c and main (spec §4.1) ------------------------------------ */

/* `priv extern_c [type_h]` asks for the interface and the implementation at
   once (type-layer-on-priv-extern-c), and `main` is not defined inside one
   (main-in-extern-c): it is a function of the module, which takes its prefix. */
static void check_extern_c(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t kw = n->first;
    while (kw < n->end && !isl_word(a, kw, "extern_c")) kw++;
    if (!n->is_public)
        for (size_t j = kw; j < n->end && j < n->body_first; j++)
            if (isl_word(a, j, "type_h")) { isl_diag3(c, K_DIAG_TYPE_LAYER_ON_PRIV_EXTERN_C, isl_tok(a, kw), none, none, none); break; }
    if (n->body_first == SIZE_MAX) return;
    size_t depth = 0, top = isl_punct(a, n->body_first, "{") ? 1 : 0;   /* the body's span may hold its braces */
    for (size_t j = n->body_first; j < n->body_end; j++) {
        if (isl_punct(a, j, "{") || isl_punct(a, j, "(")) depth++;
        else if (isl_punct(a, j, "}") || isl_punct(a, j, ")")) depth--;
        else if (depth == top && isl_word(a, j, "main") && isl_punct(a, j + 1, "(")) {
            size_t cp = isl_close_paren(a, j + 1, n->body_end);
            if (cp != SIZE_MAX && isl_punct(a, cp + 1, "{")) isl_diag3(c, K_DIAG_MAIN_IN_EXTERN_C, isl_tok(a, j), none, none, none);
        }
    }
}

/* `main` is public (private-main) and has one of the two forms of C:
   `int main(void)` or `int main(int argc, char *argv[])` (invalid-main-signature) */
static void check_main(Ctx *c, const KAstNode *fn) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    KToken name = isl_tok(a, fn->name_first);
    if (!k_token_spelled(name, "main")) return;
    if (!fn->is_public) isl_diag3(c, K_DIAG_PRIVATE_MAIN, name, none, none, none);
    size_t s = fn->first;
    while (s < fn->name_first && (isl_word(a, s, "pub") || isl_word(a, s, "priv"))) s++;
    bool ok = s + 1 == fn->name_first && isl_word(a, s, "int") && isl_punct(a, fn->name_first + 1, "(");
    size_t open = fn->name_first + 1, close = ok ? isl_close_paren(a, open, fn->end) : SIZE_MAX;
    if (close == SIZE_MAX) ok = false;
    else if (close == open + 1 || (close == open + 2 && isl_word(a, open + 1, "void"))) { /* () and (void) */ }
    else {
        /* int NAME , char * NAME [ ]   or   int NAME , char * * NAME */
        size_t j = open + 1;
        ok = isl_word(a, j, "int") && isl_ident(a, j + 1) && isl_punct(a, j + 2, ",") && isl_word(a, j + 3, "char") && isl_punct(a, j + 4, "*");
        if (ok && isl_punct(a, j + 5, "*")) ok = isl_ident(a, j + 6) && j + 7 == close;
        else if (ok) ok = isl_ident(a, j + 5) && isl_punct(a, j + 6, "[") && isl_punct(a, j + 7, "]") && j + 8 == close;
    }
    if (!ok) isl_diag3(c, K_DIAG_INVALID_MAIN_SIGNATURE, name, none, none, none);
}

/* What the v0 grammar recognizes in a signature only to refuse or place it:
   `keel_code` and the mold's `dim` parameter are v1 (spec §4.13), and a role
   stands before a parameter's type, or `child` before the return type
   (spec §4.12). */
static void check_signature(Ctx *c, const KAstNode *fn) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    for (size_t j = fn->first; j < fn->name_first; j++) {
        if (isl_word(a, j, "keel_code")) isl_diag3(c, K_DIAG_NOT_IN_V0, isl_tok(a, j), isl_tok(a, j), none, none);
        KRole r = k_token_role(isl_tok(a, j));
        if (r != K_ROLE_NONE && r != K_ROLE_CHILD) isl_diag3(c, K_DIAG_ROLE_POSITION, isl_tok(a, j), isl_tok(a, j), none, none);
    }
    Param p[K_PARAMS_MAX];
    size_t n = isl_params_of(a, fn, p, K_PARAMS_MAX);
    for (size_t k = 0; n != SIZE_MAX && k < n; k++) {
        size_t lead = p[k].role != K_ROLE_NONE ? p[k].first + 1 : p[k].first;
        if (isl_word(a, lead, "keel_code") || (isl_word(a, lead, "dim") && lead + 2 == p[k].end && isl_ident(a, lead + 1)))
            isl_diag3(c, K_DIAG_NOT_IN_V0, isl_tok(a, lead), isl_tok(a, lead), none, none);
        if (p[k].role != K_ROLE_NONE && p[k].is_type)
            isl_diag3(c, K_DIAG_ROLE_POSITION, isl_tok(a, p[k].first), isl_tok(a, p[k].first), none, none);
    }
}

bool k_collect_islands(KAst *a, KDiagnosticSink *diag) {
    a->islands.len = 0;
    a->island_text.len = 0;
    a->closure_text.len = 0;
    a->closure.len = 0;
    a->unavailable.len = 0;
    seed_count = seed_node_count = 0;
    if (!a->symbols) return true;
    static Surface surface[K_SURFACE_MAX];
    Ctx ctx = { .ast = a, .diag = diag, .surface = surface };
    isl_check_modifiers(&ctx);
    isl_check_declarations(&ctx);
    isl_check_protocols(&ctx);
    if (isl_generic(a)) {
        isl_check_generic(&ctx);
        return true;
    }
    isl_check_collisions(&ctx);
    isl_check_alias_types(&ctx);
    isl_check_layout(&ctx);
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind == K_AST_INSTANCE) isl_instance_decl(&ctx, n);
        if (n->kind == K_AST_EXTERN_C) check_extern_c(&ctx, n);
        if (n->kind == K_AST_FUNCTION && n->name_first != n->name_end) check_main(&ctx, n);
        if (n->kind == K_AST_FUNCTION) isl_check_jumps(&ctx, n);
        {
            size_t f = n->first;
            while (f < n->end && (isl_word(a, f, "pub") || isl_word(a, f, "priv") || isl_word(a, f, "inline"))) f++;
            if (n->kind != K_AST_MODULE && isl_word(a, f, "defer"))
                isl_diag3(&ctx, K_DIAG_DEFER_AT_FILE_SCOPE, isl_tok(a, f), (keel_slice_char){0}, (keel_slice_char){0},
                      (keel_slice_char){0});
        }
        if (n->kind == K_AST_CONSTEXPR) {
            size_t kw = n->first;
            while (kw < n->end && !isl_word(a, kw, "constexpr")) kw++;
            isl_check_constexpr(&ctx, kw, n->end);
        }
    }
    /* what the file declares at file scope is known to every function, wherever
       it stands in the file: read those declarations first, printing nothing */
    ctx.quiet = true;
    size_t seen = 0;
    for (size_t i = 0; i < a->nodes.len && !ctx.failed; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind != K_AST_VARIABLE || n->first < seen) continue;
        seen = n->end;
        isl_walk(&ctx, n);
        for (size_t k = ctx.global_count; k < ctx.local_count && ctx.global_count < K_LOCALS_MAX; k++)
            if (ctx.locals[k].depth == 0) {
                ctx.globals[ctx.global_count] = ctx.locals[k];
                ctx.globals[ctx.global_count++].global = true;
            }
    }
    ctx.quiet = false;
    for (size_t i = 0; i < a->nodes.len; i++)
        if (isl_node_at(a, i)->kind == K_AST_FUNCTION) check_signature(&ctx, isl_node_at(a, i));
    for (size_t i = 0; i < a->nodes.len && !ctx.failed; i++)
        if (isl_node_at(a, i)->kind == K_AST_TYPE) {
            ctx.local_count = ctx.global_count;
            memcpy(ctx.locals, ctx.globals, ctx.global_count * sizeof *ctx.globals);
            isl_extent_island(&ctx, isl_node_at(a, i));
        }
    size_t covered = 0;
    for (size_t i = 0; i < a->nodes.len && !ctx.failed; i++) {
        const KAstNode *n = isl_node_at(a, i);
        if (n->kind != K_AST_FUNCTION && n->kind != K_AST_VARIABLE) continue;
        if (n->first < covered) continue;       /* declarators of one declaration share its span */
        covered = n->end;
        isl_walk(&ctx, n);
    }
    if (!ctx.failed) close_instances(&ctx);
    return !ctx.failed;
}
