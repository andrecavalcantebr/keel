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
#include "engine/parser.h"

#define K_LOCALS_MAX 128
#define K_PARAMS_MAX 16
/* a written type, an arrow and a symbol of up to 255 characters (backend §2.1),
   with room for the marks */
#define K_DETAIL_MAX 1024

typedef struct {
    keel_slice_char name;
    KSpecifier spec;
    int pointers;               /* leading '*' of the declarator, or 1 for an array */
    int depth;                  /* brace depth of the block that declared it */
    size_t decl_at;             /* token index of the declarator's name */
    int rank;                   /* dimensions of an `array`; 0 for anything else */
    keel_slice_char element;    /* an `array`'s element type, as written */
    keel_slice_char dims;       /* its dimensions, as written: `[2,3,4]` */
    bool global;                /* declared at file scope: also a name of the module */
    bool is_ref;                /* declared with `ref` */
    bool is_constexpr;          /* a block-level `constexpr` */
    keel_slice_char value;      /* its initializer, as written */
} Local;

#define K_TYPES_MAX 256
#define K_SYMBOL_MAX 256

typedef enum { KT_NONE, KT_PRIM, KT_NAMED, KT_MODIFIER, KT_ARRAY } KTypeKind;

/* What this pass knows of the type of an expression in container position (spec
   §2.2, `container`): built from the declarations it has seen, never from C.
   Types refer to their arguments and element by index in the pool of the
   resolution that made them. */
typedef struct {
    KTypeKind kind;
    const KModule *module;      /* the module that declares the modifier or the named type */
    keel_slice_char name;       /* the name it is declared with */
    int argc;
    int arg[K_SPEC_MAX_ARGS];   /* KT_MODIFIER: the arguments, in the order of the module line */
    int pointers;               /* stars between the type and the value */
    bool lvalue;                /* the value has an address: a name, an element, a field */
    int rank;                   /* KT_ARRAY */
    keel_slice_char dims;
    int elem;                   /* KT_ARRAY: the element */
    char symbol[K_SYMBOL_MAX];  /* the C symbol of the type; empty when it cannot be named */
} KType;

typedef struct {
    KAst *ast;
    KDiagnosticSink *diag;
    KType types[K_TYPES_MAX];       /* the pool of the resolution under way */
    int type_count;
    int nest;
    Local locals[K_LOCALS_MAX];
    size_t local_count;
    Local globals[K_LOCALS_MAX];    /* what the file declares at file scope */
    size_t global_count;
    bool quiet;                     /* reading declarations, not printing islands */
    int depth;
    bool failed;
    bool has_target;            /* an initializer is being read... */
    KSpecifier target;          /* ...of an object declared with this type */
    size_t par_open, par_close; /* the worker body of the `parallel` under way, or 0, 0 */
    keel_slice_char par_name;
    size_t cap_first, cap_end;  /* its capture list, tokens inside the parentheses */
    keel_slice_char par_names[16];  /* the `parallel`s of the function so far */
    size_t par_count;
    struct Surface *surface;    /* what each instance's verbs are, computed on demand (stage 4i) */
    size_t surface_count;
} Ctx;

/* Whether a verb survives in an instance (spec §4.3, rules 12 and 13; parser
   §5): memoized by the instance's symbol and the verb's declaration. */
#define K_SURFACE_MAX 192
typedef struct Surface {
    char inst[K_SYMBOL_MAX];
    const KAstNode *fn;
    signed char state;          /* 0 being computed, 1 available, 2 unavailable */
    char cause[160];
} Surface;

typedef struct { char s[K_DETAIL_MAX]; size_t n; bool ok; } Text;

typedef struct { size_t first, end; } Range;      /* tokens [first, end) */

typedef struct {
    bool is_type;               /* `type X` */
    bool is_array;              /* `array T v[size_t N]` */
    bool is_protocol;           /* `Proto [C] x`: a function over the protocol (spec §4.14) */
    KRole role;                 /* the role written before it (spec §4.12) */
    bool pointer;               /* declared as pointer */
    keel_slice_char base;       /* first type name of the parameter; the element of an array */
    size_t first, end;          /* its tokens, in the module that declares it */
} Param;

static KToken tok(const KAst *a, size_t i) { return keel_buffer_KLexeme_ptr(&a->tokens, i)->token; }
static bool live(const KAst *a, size_t i) {
    return i < a->tokens.len && !keel_buffer_KLexeme_ptr(&a->tokens, i)->directive;
}
static bool punct(const KAst *a, size_t i, const char *s) { return live(a, i) && k_token_is_punct(tok(a, i), s); }
static bool word(const KAst *a, size_t i, const char *s) { return live(a, i) && k_token_spelled(tok(a, i), s); }
static bool ident(const KAst *a, size_t i) { return live(a, i) && k_token_is_ident(tok(a, i)); }

/* The first token whose spelling starts at or after `ptr`. */
static size_t index_at(const KAst *a, size_t from, const char *ptr) {
    size_t k = from;
    while (k < a->tokens.len && tok(a, k).ptr < ptr) k++;
    return k;
}

static KToken enter(const KAst *a, size_t i, KLexer *lexer, TKPpKind *pp) {
    KToken t = tok(a, i);
    k_lexer_init(lexer, (keel_slice_char){ (size_t)(a->source.ptr + a->source.len - t.ptr), t.ptr }, NULL);
    return k_lexer_next(lexer, pp);
}

/* the index of the ')' that closes the '(' at `open`; SIZE_MAX if none in [open, end) */
static size_t close_paren(const KAst *a, size_t open, size_t end) {
    size_t depth = 0;
    for (size_t j = open; j < end; j++) {
        if (punct(a, j, "(")) depth++;
        else if (punct(a, j, ")") && --depth == 0) return j;
    }
    return SIZE_MAX;
}

static size_t close_bracket(const KAst *a, size_t open, size_t end) {
    size_t depth = 0;
    for (size_t j = open; j < end; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) {
            if (--depth == 0) return punct(a, j, "]") ? j : SIZE_MAX;
        }
    }
    return SIZE_MAX;
}

/* ---- the detail column ------------------------------------------------- */

static void put(Text *t, const char *p, size_t n) {
    if (n > sizeof t->s - t->n) { t->ok = false; return; }
    memcpy(t->s + t->n, p, n);
    t->n += n;
}
static void put_str(Text *t, const char *s) { put(t, s, strlen(s)); }
static void put_uint(Text *t, size_t v) {
    char d[24]; size_t k = 0;
    do { d[k++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (k) put(t, &d[--k], 1);
}
/* tokens as the dump prints them: one space between, none around `.`, `(`, `)`, `,` */
static void put_tokens(Text *t, const KAst *a, size_t first, size_t end) {
    for (size_t j = first; j < end; j++) {
        KToken k = tok(a, j);
        if (j > first) {
            KToken p = tok(a, j - 1);
            bool dot = k_token_is_punct(k, ".") || k_token_is_punct(p, ".");
            bool bracket = k_token_is_punct(k, "(") || k_token_is_punct(k, ")") ||
                           k_token_is_punct(k, ",") || k_token_is_punct(p, "(");
            if (!dot && !bracket) put(t, " ", 1);
        }
        put(t, k.ptr, k.len);
    }
}
static void put_glued(Text *t, const KAst *a, size_t first, size_t end) {
    for (size_t j = first; j < end; j++) put(t, tok(a, j).ptr, tok(a, j).len);
}
/* source text, its white space collapsed to one space and trimmed at both ends */
static void put_source(Text *t, const char *p, const char *end) {
    bool space = false, any = false;
    for (; p < end; p++) {
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') { space = any; continue; }
        if (space) put(t, " ", 1);
        put(t, p, 1);
        space = false;
        any = true;
    }
}

/* islands stay sorted by anchor: a later declarator's island can be found
   before an earlier declarator's initializer is read */
/* A diagnostic argument built here must outlive the function that built it:
   the sink formats the message later. It is kept in the island text, after
   the details, where nothing reads it but the diagnostic. */
static keel_slice_char keep(Ctx *c, const Text *t) {
    KAst *a = c->ast;
    if (!t->ok || t->n > a->island_text.cap - a->island_text.len) return k_diag_text("its verbs");
    char *at = a->island_text.ptr + a->island_text.len;
    memcpy(at, t->s, t->n);
    a->island_text.len += t->n;
    return (keel_slice_char){ t->n, at };
}

static bool emit(Ctx *c, KIslandKind kind, size_t anchor, const Text *t) {
    KAst *a = c->ast;
    if (c->quiet) return true;
    if (!t->ok || a->islands.len == a->islands.cap || t->n > a->island_text.cap - a->island_text.len) {
        if (!c->failed) {
            KToken at = tok(a, anchor);
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

static const Local *find_local(const Ctx *c, keel_slice_char name) {
    for (size_t i = c->local_count; i-- > 0;)
        if (k_symtab_same_name(c->locals[i].name, name)) return &c->locals[i];
    return NULL;
}

/* The value of a decimal literal, or of a name whose initializer is one (spec
   §4.2, item 14): keel does not calculate expressions. */
static bool digits(keel_slice_char s, long *out) {
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

static bool decimal_of_token(const Ctx *c, KToken t, long *out) {
    if (k_token_is_number(t)) return digits(t, out);
    if (!k_token_is_ident(t)) return false;
    const Local *l = find_local(c, t);
    if (l) return l->is_constexpr && digits(l->value, out);
    const KSymbol *sym = k_symbol_resolve(c->ast->symbols, t);
    return sym && sym->kind == K_SYM_CONSTANT && digits(sym->value, out);
}

static bool decimal_of(const Ctx *c, Range r, long *out) {
    return r.end == r.first + 1 && decimal_of_token(c, tok(c->ast, r.first), out);
}

/* The dimensions an `array` declares, in order, from the text `[2,3][4]`; -1 for
   one that is not a known decimal (a binder, an expression). */
static size_t dims_of(const Ctx *c, keel_slice_char text, long out[8]) {
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
            if (n < 8) out[n++] = tokens == 1 && decimal_of_token(c, only, &v) ? v : -1;
            continue;
        }
        if (depth == 1 && k_token_is_punct(t, ",")) {
            long v;
            if (n < 8) out[n++] = tokens == 1 && decimal_of_token(c, only, &v) ? v : -1;
            tokens = 0;
            continue;
        }
        if (depth >= 1) { tokens++; only = t; }
    }
    return n;
}

static void add_local(Ctx *c, Local local) {
    if (c->local_count == K_LOCALS_MAX) {
        if (!c->failed) {
            k_diag_emit(c->diag, K_DIAG_CAPACITY, local.name, (KDiagArgs){{local.name}});
            c->failed = true;
        }
        return;
    }
    local.depth = c->depth;
    c->locals[c->local_count++] = local;
}

/* The dimensions a `[...]` part declares: one per comma inside a group, one
   per group — `[2,3,4]` and `[2][3][4]` are both rank 3. */
static int rank_of(const char *p, const char *end) {
    int rank = 0, depth = 0;
    for (; p < end; p++) {
        if (*p == '[' || *p == '(' || *p == '{') { if (depth++ == 0 && *p == '[') rank++; }
        else if (*p == ']' || *p == ')' || *p == '}') depth--;
        else if (*p == ',' && depth == 1) rank++;
    }
    return rank;
}

/* ---- the module a call lands in ---------------------------------------- */

static const KAstNode *node_at(const KAst *a, size_t i) { return keel_buffer_KAstNode_ptr(&a->nodes, i); }

static const KAstNode *module_node(const KAst *a) { return node_at(a, a->module); }

static bool generic(const KAst *a) {
    const KAstNode *m = module_node(a);
    return m->dim_first != m->dim_end || m->tags_first != m->tags_end || m->type_first != m->type_end;
}

/* `keel.arena` → keel_arena */
static void put_module_prefix(Text *t, const KAst *a) {
    const KAstNode *m = module_node(a);
    for (size_t j = m->name_first; j < m->name_end; j++) {
        if (punct(a, j, ".")) put(t, "_", 1);
        else put(t, tok(a, j).ptr, tok(a, j).len);
    }
}

static bool module_named(const KAst *a, const char *name) {
    const KAstNode *m = module_node(a);
    char buf[64]; size_t n = 0;
    for (size_t j = m->name_first; j < m->name_end; j++) {
        KToken k = tok(a, j);
        if (k.len > sizeof buf - n) return false;
        memcpy(buf + n, k.ptr, k.len); n += k.len;
    }
    return n == strlen(name) && memcmp(buf, name, n) == 0;
}

static bool binder(const KAst *a, keel_slice_char name) {
    const KAstNode *m = module_node(a);
    for (size_t j = m->type_first; j < m->type_end; j++)
        if (ident(a, j) && k_symtab_same_name(tok(a, j), name)) return true;
    return false;
}

static keel_slice_char modifier_of(const KAst *a) {
    for (size_t i = 0; i < a->nodes.len; i++)
        if (node_at(a, i)->kind == K_AST_MODIFIER)
            return tok(a, node_at(a, i)->name_first);
    return (keel_slice_char){0};
}

/* The parameters of the function node `fn`, in `a`. SIZE_MAX if they do not fit. */
static size_t params_of(const KAst *a, const KAstNode *fn, Param *out, size_t cap) {
    size_t open = fn->name_end, n = 0, depth = 0, start = open + 1;
    if (!punct(a, open, "(")) return 0;
    for (size_t j = open + 1; j < a->tokens.len; j++) {
        bool closing = false;
        if (punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, "]") || punct(a, j, ")")) {
            if (depth) depth--;
            else if (punct(a, j, ")")) closing = true;
        }
        if (!closing && !(depth == 0 && punct(a, j, ","))) continue;
        if (j > start && !(j == start + 1 && word(a, start, "void"))) {
            if (n == cap) return SIZE_MAX;
            /* a role leads the parameter, and is not its type (spec §4.12) */
            KRole role = k_token_role(tok(a, start));
            size_t lead = role != K_ROLE_NONE ? start + 1 : start;
            Param p = { .is_type = word(a, lead, "type"), .is_array = word(a, lead, "array"),
                        .role = role, .first = start, .end = j };
            for (size_t k = lead; k < j; k++) {
                if (punct(a, k, "*") || (punct(a, k, "[") && !p.is_array)) p.pointer = true;
                if (!p.base.len && ident(a, k) && !((p.is_type || p.is_array) && k == lead)) p.base = tok(a, k);
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

typedef struct {
    Param p[K_PARAMS_MAX];
    size_t n;
    bool found;
    bool overloaded;            /* declared in more than one arity (backend §2.1.1) */
    bool producer;              /* returns the module's own type: `of`, `from`, `clone` (spec §4.4, item 1) */
    const KAstNode *fn;         /* the declaration found */
} Sig;

/* The declaration of `verb` in `a` with `arity` parameters (any arity when
   `arity` is SIZE_MAX), and whether the name is declared in several. */
static Sig find_verb(const KAst *a, keel_slice_char verb, size_t arity) {
    Sig sig = {0};
    uint64_t seen = 0;
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *fn = node_at(a, i);
        if (fn->kind != K_AST_FUNCTION || !k_symtab_same_name(tok(a, fn->name_first), verb)) continue;
        Param p[K_PARAMS_MAX];
        size_t n = params_of(a, fn, p, K_PARAMS_MAX);
        if (n == SIZE_MAX) continue;
        seen |= (uint64_t)1 << n;
        if (!sig.found && (arity == SIZE_MAX || n == arity)) {
            memcpy(sig.p, p, n * sizeof *p);
            sig.n = n;
            sig.found = true;
            sig.fn = fn;
            keel_slice_char own = modifier_of(a);
            for (size_t j = fn->first; own.len && j < fn->name_first; j++)
                if (ident(a, j) && k_symtab_same_name(tok(a, j), own)) sig.producer = true;
        }
    }
    sig.overloaded = (seen & (seen - 1)) != 0;
    return sig;
}

static const KModule *module_alias(const KSymbolTable *symbols, keel_slice_char name) {
    for (size_t i = 0; i < symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin && k_symtab_same_name(s->name, name)) return s->origin;
    }
    return NULL;
}

/* the module a file imports under its own declared name, whatever the alias */
static const KModule *module_by_name(const KSymbolTable *symbols, const char *name) {
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
static bool protocol_decl(const Ctx *c, const KAst *home, keel_slice_char name, KProtocolDecl *out, int depth) {
    if (depth > 4) return false;
    const KAst *where = home;
    const KSymbol *s = home->symbols ? k_symtab_lookup(home->symbols, name) : NULL;
    if (s && s->origin && s->origin->ast) where = s->origin->ast;
    for (size_t i = 0; i < where->nodes.len; i++) {
        const KAstNode *n = node_at(where, i);
        if (n->kind != K_AST_PROTOCOL || !k_symtab_same_name(tok(where, n->name_first), name)) continue;
        size_t kw = n->first;
        while (kw < n->name_first && !word(where, kw, "protocol")) kw++;
        KToken t = tok(where, kw);
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
            if (!protocol_decl(c, where, out->components[k], &part, depth + 1)) return false;
            for (size_t j = 0; j < part.verb_count && out->verb_count < K_PROTOCOL_MAX_ITEMS; j++)
                out->verbs[out->verb_count++] = part.verbs[j];
            for (size_t j = 0; j < part.assoc_count && out->assoc_count < K_PROTOCOL_MAX_ITEMS; j++)
                out->assoc[out->assoc_count++] = part.assoc[j];
        }
        return true;
    }
    return false;
}

static Sig find_verb(const KAst *a, keel_slice_char verb, size_t arity);
static void diag3(Ctx *c, KDiagId id, keel_slice_char at, keel_slice_char x, keel_slice_char y,
                  keel_slice_char z);

/* Whether the module `own` declares every verb of `pd` (spec §5.1, rule 3),
   naming the ones it lacks in `missing`; a verb whose roles differ from the
   prototype's is protocol-role-mismatch (rule 10). */
static bool satisfies(Ctx *c, const KAst *own, keel_slice_char receiver, const KProtocolDecl *pd, Text *missing, KToken at) {
    bool ok = true;
    for (size_t i = 0; i < pd->verb_count; i++) {
        const KProtocolVerb *v = &pd->verbs[i];
        Sig sig = find_verb(own, v->name, v->arity);
        /* the verb must take this type as its receiver, and not another of the module's */
        if (sig.found && receiver.len && v->receiver < sig.n && !sig.p[v->receiver].is_array &&
            !k_symtab_same_name(sig.p[v->receiver].base, receiver))
            sig.found = false;
        if (!sig.found) {
            if (missing->n) put_str(missing, ", ");
            put(missing, v->name.ptr, v->name.len);
            ok = false;
            continue;
        }
        bool written = false, differ = false;
        for (size_t k = 0; k < sig.n && k < K_PROTOCOL_MAX_PARAMS; k++) {
            if (sig.p[k].role != K_ROLE_NONE) written = true;
            if (sig.p[k].role != v->roles[k]) differ = true;
        }
        if (written && differ)
            diag3(c, K_DIAG_PROTOCOL_ROLE_MISMATCH, at, v->name, pd->name, (keel_slice_char){0});
    }
    return ok;
}

/* The associated type the function `fn` returns, written `C.name`; empty when
   its return type is not one. */
static keel_slice_char returned_associated(const KAst *home, const KAstNode *fn) {
    for (size_t j = fn->first; j + 2 < fn->name_first; j++)
        if (ident(home, j) && punct(home, j + 1, ".") && ident(home, j + 2)) return tok(home, j + 2);
    return (keel_slice_char){0};
}

static bool returns_associated(const KAst *home, const KAstNode *fn) {
    return returned_associated(home, fn).len != 0;
}

/* The verb of `pd` the call is resolved as: the one whose result is the
   associated type the function returns, or else the first that takes the
   receiver. */
static const KProtocolVerb *verb_for_return(const KAst *home, const KAstNode *fn, const KProtocolDecl *pd) {
    keel_slice_char assoc = returned_associated(home, fn);
    for (size_t i = 0; assoc.len && i < pd->verb_count; i++)
        if (k_symtab_same_name(pd->verbs[i].returns, assoc)) return &pd->verbs[i];
    for (size_t i = 0; i < pd->verb_count; i++)
        if (pd->verbs[i].receiver == 0) return &pd->verbs[i];
    return NULL;
}

/* ---- calls ------------------------------------------------------------- */

static keel_slice_char range_text(const KAst *a, Range r) {
    if (r.first >= r.end) return (keel_slice_char){0};
    KToken f = tok(a, r.first), l = tok(a, r.end - 1);
    return (keel_slice_char){ (size_t)(l.ptr + l.len - f.ptr), f.ptr };
}

static void diag3(Ctx *c, KDiagId id, keel_slice_char at, keel_slice_char x, keel_slice_char y,
                  keel_slice_char z) {
    if (c->quiet) return;
    k_diag_emit(c->diag, id, at, (KDiagArgs){{ x, y, z }});
}

/* The shortest name a module is imported under: the alias, when there is one. */
static keel_slice_char alias_of(const KSymbolTable *symbols, const KModule *m) {
    keel_slice_char best = m->name;
    for (size_t i = 0; i < symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin == m && s->name.len < best.len) best = s->name;
    }
    return best;
}

/* the first token of a piece of source text */
static KToken enter_text(keel_slice_char text, KLexer *lexer, TKPpKind *pp) {
    k_lexer_init(lexer, text, NULL);
    return k_lexer_next(lexer, pp);
}

typedef enum { SHAPE_NAME, SHAPE_CONTAINER, SHAPE_C } Shape;

/* What the `container` production (spec §2.2) makes of an argument: one
   identifier, a shape it admits but this pass cannot type (a field, an index,
   a verb over a container), or C — a literal, an operator, a cast, a call of
   anything but a qualified verb — which gives no container identity (§4.4). */
static Shape container_shape(const KAst *a, Range r) {
    if (r.end == r.first + 1 && ident(a, r.first)) return SHAPE_NAME;
    size_t depth = 0;
    for (size_t j = r.first; j < r.end; j++) {
        if (punct(a, j, "(")) {
            if (depth == 0) {
                size_t k = close_paren(a, j, r.end);
                if (k != SIZE_MAX && k + 1 < r.end &&
                    (ident(a, k + 1) || k_token_is_number(tok(a, k + 1)))) return SHAPE_C;   /* a cast */
                bool verb = j >= r.first + 3 && ident(a, j - 1) && punct(a, j - 2, ".") && ident(a, j - 3);
                if (j > r.first && ident(a, j - 1) && !verb) return SHAPE_C;                  /* a C call */
            }
            depth++;
        } else if (punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) { if (depth) depth--; }
        else if (depth == 0) {
            KToken t = tok(a, j);
            if (k_token_is_number(t) || k_token_is_string(t) || k_token_is_char(t)) return SHAPE_C;
            bool prefix = j == r.first && (punct(a, j, "*") || punct(a, j, "&"));
            if (!(ident(a, j) || punct(a, j, ".") || punct(a, j, "->") || prefix)) return SHAPE_C;
        }
    }
    return SHAPE_CONTAINER;
}

static const Local *arg_local(const Ctx *c, Range r) {
    return r.end == r.first + 1 && ident(c->ast, r.first) ? find_local(c, tok(c->ast, r.first)) : NULL;
}

/* ---- container types ---------------------------------------------------- */

static KType *kt(Ctx *c, int t) { return &c->types[t]; }

/* the symbol of a type in the pool; empty for one that could not be made */
static const char *kt_symbol(const Ctx *c, int t) { return t >= 0 ? c->types[t].symbol : ""; }

static int kt_new(Ctx *c) {
    if (c->type_count == K_TYPES_MAX) return -1;
    c->types[c->type_count] = (KType){ .elem = -1 };
    return c->type_count++;
}

static int kt_copy(Ctx *c, int t) {
    if (t < 0) return -1;
    int n = kt_new(c);
    if (n >= 0) c->types[n] = c->types[t];
    return n;
}

static void kt_set_symbol(KType *k, const char *s, size_t n) {
    if (n < K_SYMBOL_MAX) { memcpy(k->symbol, s, n); k->symbol[n] = 0; }
}

/* the module's own AST; the file being read has no KModule yet */
static const KAst *ast_of(const Ctx *c, const KModule *m) { return m && m->ast ? m->ast : c->ast; }

/* the C symbol of a modifier's instance: the modifier's name and its arguments' */
static void kt_compose(Ctx *c, int t, const KSymbol *modifier) {
    KType *k = kt(c, t);
    char buf[K_SYMBOL_MAX];
    size_t n = k_mangle_symbol(modifier, (keel_slice_char){ sizeof buf, buf });
    if (n > sizeof buf) return;
    for (int i = 0; i < k->argc; i++) {
        const char *s = kt_symbol(c, k->arg[i]);
        size_t len = strlen(s);
        if (!len || n + 1 + len >= sizeof buf) return;
        buf[n++] = '_';
        memcpy(buf + n, s, len);
        n += len;
    }
    kt_set_symbol(k, buf, n);
}

/* A type written in `symbols`' scope. */
static int kt_from_text(Ctx *c, const KSymbolTable *symbols, keel_slice_char text) {
    int t = kt_new(c);
    if (t < 0) return -1;
    char sym[K_SYMBOL_MAX];
    size_t n = k_spec_symbol(text, symbols, sym, sizeof sym);
    if (n != SIZE_MAX) kt_set_symbol(kt(c, t), sym, n);
    KLexer lexer;
    TKPpKind pp;
    KToken first = enter_text(text, &lexer, &pp);
    while (first.len && (k_token_spelled(first, "const") || k_token_spelled(first, "volatile") ||
                         k_token_spelled(first, "_Atomic")))
        first = k_lexer_next(&lexer, &pp);
    KSpecifier spec;
    KToken next = {0};
    if (!first.len || !k_token_is_ident(first) ||
        !k_scan_known_type(&lexer, first, symbols, &spec, &next, &pp) || spec.kind == K_SPEC_NONE) {
        kt(c, t)->kind = n != SIZE_MAX ? KT_PRIM : KT_NONE;       /* `char`, `struct S`, ... */
        return t;
    }
    if (spec.kind == K_SPEC_NAMED_TYPE) {
        const KSymbol *s = k_symbol_resolve(symbols, spec.type_name);
        kt(c, t)->kind = k_primitive_name(spec.type_name) ? KT_PRIM : KT_NAMED;
        if (s) { kt(c, t)->module = s->origin; kt(c, t)->name = s->name; }
        return t;
    }
    const KSymbol *m = k_symbol_resolve(symbols, spec.modifier_name);
    kt(c, t)->kind = KT_MODIFIER;
    if (m) { kt(c, t)->module = m->origin; kt(c, t)->name = m->name; }
    for (size_t i = 0; i < spec.arg_count; i++) {
        int arg = kt_from_text(c, symbols, spec.args[i]);
        if (kt(c, t)->argc < K_SPEC_MAX_ARGS) kt(c, t)->arg[kt(c, t)->argc++] = arg;
    }
    return t;
}

static int kt_of_local(Ctx *c, const Local *l) {
    int t;
    if (l->rank > 0) {
        t = kt_new(c);
        if (t < 0) return -1;
        int element = kt_from_text(c, c->ast->symbols, l->element);
        kt(c, t)->kind = KT_ARRAY;
        kt(c, t)->rank = l->rank;
        kt(c, t)->dims = l->dims;
        kt(c, t)->elem = element;
    } else if (l->spec.kind == K_SPEC_NONE) {
        t = kt_new(c);
    } else {
        t = kt_from_text(c, c->ast->symbols, l->spec.text);
    }
    if (t >= 0) {
        kt(c, t)->pointers = l->rank > 0 ? 0 : l->pointers;
        kt(c, t)->lvalue = true;
    }
    return t;
}

/* the type parameters of a generic module, in the order of its `module` line */
static size_t binders_of(const KAst *home, keel_slice_char out[8]) {
    const KAstNode *m = module_node(home);
    size_t n = 0;
    for (size_t j = m->tags_first; j < m->tags_end; j++) if (ident(home, j) && n < 8) out[n++] = tok(home, j);
    for (size_t j = m->type_first; j < m->type_end; j++) if (ident(home, j) && n < 8) out[n++] = tok(home, j);
    return n;
}

/* A type written in a generic module `home`, seen from an instance of it: the
   type parameters are the instance's arguments, and the module's own modifier,
   or an imported one written without arguments, takes them as they stand. */
static int kt_from_home(Ctx *c, const KAst *home, keel_slice_char text, int inst) {
    KLexer lexer;
    TKPpKind pp;
    KToken first = enter_text(text, &lexer, &pp);
    while (first.len && (k_token_spelled(first, "const") || k_token_spelled(first, "volatile") ||
                         k_token_spelled(first, "_Atomic")))
        first = k_lexer_next(&lexer, &pp);
    if (!first.len) return -1;
    keel_slice_char body = { (size_t)(text.ptr + text.len - first.ptr), first.ptr };
    KToken after = k_lexer_next(&lexer, &pp);
    keel_slice_char binders[8];
    size_t nb = binders_of(home, binders);

    if (!after.len) {                                   /* one name */
        for (size_t p = 0; p < nb; p++)
            if (k_symtab_same_name(binders[p], first)) {
                if (inst < 0 || p >= (size_t)kt(c, inst)->argc) return -1;
                return kt_copy(c, kt(c, inst)->arg[p]);
            }
        if (k_symtab_same_name(first, modifier_of(home))) return kt_copy(c, inst);
        const KSymbol *s = k_symbol_resolve(home->symbols, first);
        if (s && s->kind == K_SYM_MODIFIER && s->origin) {
            int t = kt_new(c);
            if (t < 0) return -1;
            kt(c, t)->kind = KT_MODIFIER;
            kt(c, t)->module = s->origin;
            kt(c, t)->name = s->name;
            for (int i = 0; i < s->arity - s->dim_arity && i < K_SPEC_MAX_ARGS; i++) {
                int arg = inst >= 0 && i < kt(c, inst)->argc ? kt_copy(c, kt(c, inst)->arg[i]) : -1;
                if (arg < 0) return -1;
                kt(c, t)->arg[kt(c, t)->argc++] = arg;
            }
            kt_compose(c, t, s);
            return t;
        }
        return kt_from_text(c, home->symbols, body);
    }

    KLexer again;
    KSpecifier spec;
    KToken next = {0};
    KToken head = enter_text(body, &again, &pp);
    if (!k_token_is_ident(head) || !k_scan_known_type(&again, head, home->symbols, &spec, &next, &pp) ||
        spec.kind != K_SPEC_MODIFIER) {
        /* `outcome buffer` in keel.buffer: a one-argument modifier over the
           module's own modifier, written bare, which the scanner does not take */
        const KSymbol *m = k_token_is_ident(head) ? k_symbol_resolve(home->symbols, head) : NULL;
        if (m && m->kind == K_SYM_MODIFIER && m->origin && m->arity - m->dim_arity == 1) {
            keel_slice_char rest = { (size_t)(body.ptr + body.len - after.ptr), after.ptr };
            int arg = kt_from_home(c, home, rest, inst);
            int t = arg >= 0 ? kt_new(c) : -1;
            if (t >= 0 && *kt(c, arg)->symbol) {
                kt(c, t)->kind = KT_MODIFIER;
                kt(c, t)->module = m->origin;
                kt(c, t)->name = m->name;
                kt(c, t)->arg[kt(c, t)->argc++] = arg;
                kt_compose(c, t, m);
                return t;
            }
        }
        return kt_from_text(c, home->symbols, body);
    }
    const KSymbol *m = k_symbol_resolve(home->symbols, spec.modifier_name);
    if (!m || !m->origin) return -1;
    int t = kt_new(c);
    if (t < 0) return -1;
    kt(c, t)->kind = KT_MODIFIER;
    kt(c, t)->module = m->origin;
    kt(c, t)->name = m->name;
    for (size_t i = 0; i < spec.arg_count; i++) {
        int arg = kt_from_home(c, home, spec.args[i], inst);
        if (arg < 0 || kt(c, t)->argc == K_SPEC_MAX_ARGS) return -1;
        kt(c, t)->arg[kt(c, t)->argc++] = arg;
    }
    kt_compose(c, t, m);
    return t;
}

/* the declared return type of `fn`, for an instance `inst` of the module that declares it */
static int ret_type(Ctx *c, const KAst *home, const KAstNode *fn, int inst) {
    size_t s = fn->first;
    while (s < fn->name_first && (word(home, s, "pub") || word(home, s, "priv") || word(home, s, "inline") ||
                                  word(home, s, "static") || word(home, s, "extern") ||
                                  k_token_role(tok(home, s)) == K_ROLE_CHILD))
        s++;
    size_t e = fn->name_first;
    int pointers = 0;
    while (e > s && punct(home, e - 1, "*")) { pointers++; e--; }
    if (e <= s) return -1;
    int t = kt_from_home(c, home, range_text(home, (Range){ s, e }), inst);
    if (t >= 0) {
        kt(c, t)->pointers = pointers;
        kt(c, t)->lvalue = false;
    }
    return t;
}

/* the instance of module `m` inside a type, found where a parameter that is not
   the module's own modifier still mentions it (`slice slot s`) */
static int kt_find(Ctx *c, int t, const KModule *m, int depth) {
    if (t < 0 || depth > 8) return -1;
    KType *k = kt(c, t);
    if (k->kind == KT_MODIFIER && k->module == m) return t;
    if (k->kind == KT_MODIFIER)
        for (int i = 0; i < k->argc; i++) {
            int found = kt_find(c, k->arg[i], m, depth + 1);
            if (found >= 0) return found;
        }
    if (k->kind == KT_ARRAY) return kt_find(c, k->elem, m, depth + 1);
    return -1;
}

/* A field of a struct the file or an import declares: the keel ones are
   registered with their types, the C ones are opaque (spec §4.2). */
static int scan_fields(Ctx *c, const KAst *ast, size_t first, size_t last, keel_slice_char field) {
    for (size_t j = first; j < last; ) {
        size_t e = j, depth = 0;
        while (e < last && !(depth == 0 && punct(ast, e, ";"))) {
            if (punct(ast, e, "(") || punct(ast, e, "[") || punct(ast, e, "{")) depth++;
            else if (punct(ast, e, ")") || punct(ast, e, "]") || punct(ast, e, "}")) depth--;
            e++;
        }
        size_t k = j;
        if (word(ast, k, "alignas") && punct(ast, k + 1, "(")) {
            size_t cp = close_paren(ast, k + 1, e);
            if (cp != SIZE_MAX) k = cp + 1;
        }
        KLexer lexer;
        TKPpKind pp;
        if (k < e && word(ast, k, "array")) {
            KToken next = {0}, head = enter(ast, k + 1, &lexer, &pp);
            keel_slice_char element;
            if (k_scan_argument(&lexer, head, ast->symbols, &element, &next, &pp))
                for (KToken d = next; d.len; ) {
                    KDeclarator decl;
                    KToken after;
                    if (!(k_token_is_punct(d, "*") || k_token_is_ident(d))) break;
                    if (!k_scan_declarator(&lexer, d, &decl, &after, &pp) || !decl.name.len || !decl.has_array_suffix) break;
                    if (k_symtab_same_name(decl.name, field)) {
                        const char *dims = decl.name.ptr + decl.name.len, *end = after.len ? after.ptr : dims;
                        int t = kt_new(c);
                        if (t < 0) return -1;
                        kt(c, t)->kind = KT_ARRAY;
                        kt(c, t)->rank = rank_of(dims, end);
                        kt(c, t)->dims = (keel_slice_char){ (size_t)(end - dims), (char *)dims };
                        kt(c, t)->elem = kt_from_text(c, ast->symbols, element);
                        kt(c, t)->lvalue = true;
                        return t;
                    }
                    if (!k_token_is_punct(after, ",")) break;
                    d = k_lexer_next(&lexer, &pp);
                }
        } else if (k < e && ident(ast, k)) {
            KSpecifier spec;
            KToken next = {0}, head = enter(ast, k, &lexer, &pp);
            if (k_scan_known_type(&lexer, head, ast->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE)
                for (KToken d = next; d.len; ) {
                    KDeclarator decl;
                    KToken after;
                    if (!(k_token_is_punct(d, "*") || k_token_is_ident(d))) break;
                    if (!k_scan_declarator(&lexer, d, &decl, &after, &pp) || !decl.name.len) break;
                    if (k_symtab_same_name(decl.name, field)) {
                        int t = kt_from_text(c, ast->symbols, spec.text);
                        if (t >= 0) { kt(c, t)->pointers = decl.pointer_depth; kt(c, t)->lvalue = true; }
                        return t;
                    }
                    if (!k_token_is_punct(after, ",")) break;
                    d = k_lexer_next(&lexer, &pp);
                }
        }
        j = e + 1;
    }
    return -1;
}

static int field_type(Ctx *c, int t, keel_slice_char field) {
    if (t < 0) return -1;
    KType *k = kt(c, t);
    if (k->kind != KT_NAMED || !k->name.len) return -1;
    const KAst *ast = ast_of(c, k->module);
    if (!ast || !ast->symbols) return -1;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = node_at(ast, i);
        if (n->kind != K_AST_TYPE || !k_symtab_same_name(tok(ast, n->name_first), k->name)) continue;
        size_t open = n->first;
        while (open < n->end && !punct(ast, open, "{")) open++;
        if (open == n->end) return -1;
        size_t depth = 0, close = open;
        for (; close < n->end; close++) {
            if (punct(ast, close, "{")) depth++;
            else if (punct(ast, close, "}") && --depth == 0) break;
        }
        return scan_fields(c, ast, open + 1, close, field);
    }
    return -1;
}

/* `x[i]` with `count` indices: the element of an array, or what `ptr` points to */
static int index_type(Ctx *c, int t, size_t count) {
    if (t < 0) return -1;
    KType *k = kt(c, t);
    if (k->kind == KT_ARRAY) {
        if (count > (size_t)k->rank) return -1;
        if (count == (size_t)k->rank) {
            int e = kt_copy(c, k->elem);
            if (e >= 0) kt(c, e)->lvalue = true;
            return e;
        }
        int rest = kt_copy(c, t);
        if (rest >= 0) kt(c, rest)->rank -= (int)count;
        return rest;
    }
    if (k->kind == KT_MODIFIER && k->module) {
        const KAst *home = ast_of(c, k->module);
        Sig sig = find_verb(home, k_diag_text("ptr"), count + 1);
        if (!sig.found) return -1;
        int p = ret_type(c, home, sig.fn, t);
        if (p < 0 || kt(c, p)->pointers < 1) return -1;
        kt(c, p)->pointers--;
        kt(c, p)->lvalue = true;
        return p;
    }
    return -1;
}

static bool first_arg(const KAst *a, size_t open, Range *out) {
    size_t depth = 0, j = open + 1;
    for (; j < a->tokens.len; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) { if (depth == 0) break; depth--; }
        else if (depth == 0 && punct(a, j, ",")) break;
    }
    if (j == open + 1) return false;
    *out = (Range){ open + 1, j };
    return true;
}

static int type_of(Ctx *c, Range r);
static bool tags_decl(const KAst *where, keel_slice_char name, KTagsDecl *out);
static bool tag_of_some_set(const KAst *where, keel_slice_char name);
static bool verb_available(Ctx *c, const KAst *home, const KAstNode *fn, int t, const char **cause);
static int call_core(Ctx *c, size_t callee, size_t verb, size_t open, const KModule *module);

/* How a call written at token `i` is dispatched: `alias.verb(`; `verb(x, …)`
   as the verb of the type of the container `x` (spec §4.4, item 1); or a
   function of the file itself. */
typedef struct { size_t verb, open; const KModule *module; } Callee;

static bool callee_at(Ctx *c, size_t i, Callee *out) {
    const KAst *a = c->ast;
    KToken name = tok(a, i);
    if (!ident(a, i) || find_local(c, name)) return false;
    if (punct(a, i + 1, ".") && ident(a, i + 2) && punct(a, i + 3, "(")) {
        const KModule *m = module_alias(a->symbols, name);
        if (!m) return false;
        *out = (Callee){ i + 2, i + 3, m };
        return true;
    }
    if (!punct(a, i + 1, "(")) return false;
    Range first;
    if (first_arg(a, i + 1, &first)) {
        int t = type_of(c, first);
        if (t >= 0 && kt(c, t)->kind == KT_MODIFIER && kt(c, t)->module) {
            const KAst *home = ast_of(c, kt(c, t)->module);
            if (find_verb(home, name, SIZE_MAX).found) {
                *out = (Callee){ i, i + 1, kt(c, t)->module };
                return true;
            }
        }
    }
    for (size_t k = 0; k < a->nodes.len; k++)
        if (node_at(a, k)->kind == K_AST_FUNCTION && k_symtab_same_name(tok(a, node_at(a, k)->name_first), name)) {
            *out = (Callee){ i, i + 1, NULL };
            return true;
        }
    return false;
}

/* the type of a call's result, without printing or refusing anything */
static int call_result(Ctx *c, size_t callee, size_t verb, size_t open, const KModule *module) {
    bool quiet = c->quiet;
    c->quiet = true;
    int t = call_core(c, callee, verb, open, module);
    c->quiet = quiet;
    return t;
}

/* The type of an expression in container position (spec §2.2, `container`):
   a name, `*x`, `&x`, `(x)`, `x[i]`, `x.f`, `x->f`, and a verb over a container.
   -1 when it is none of these, or names something this pass has not seen declared. */
static int type_of(Ctx *c, Range r) {
    const KAst *a = c->ast;
    while (r.end > r.first + 1 && punct(a, r.first, "(") && close_paren(a, r.first, r.end) == r.end - 1) {
        r.first++;
        r.end--;
    }
    if (r.first >= r.end) return -1;
    if (punct(a, r.first, "*") || punct(a, r.first, "&")) {
        bool star = punct(a, r.first, "*");
        int t = type_of(c, (Range){ r.first + 1, r.end });
        if (t < 0) return -1;
        if (star) {
            if (kt(c, t)->pointers < 1) return -1;
            kt(c, t)->pointers--;
            kt(c, t)->lvalue = true;
        } else {
            kt(c, t)->pointers++;
            kt(c, t)->lvalue = false;
        }
        return t;
    }
    if (!ident(a, r.first)) return -1;
    size_t j = r.first;
    int t = -1;
    Callee ce;
    if (callee_at(c, j, &ce)) {
        size_t close = close_paren(a, ce.open, r.end);
        if (close == SIZE_MAX) return -1;
        t = call_result(c, j, ce.verb, ce.open, ce.module);
        j = close + 1;
    } else {
        const Local *l = find_local(c, tok(a, j));
        if (!l) return -1;
        t = kt_of_local(c, l);
        j++;
    }
    while (t >= 0 && j < r.end) {
        if (punct(a, j, "[")) {
            size_t close = close_bracket(a, j, r.end);
            if (close == SIZE_MAX) return -1;
            size_t count = 1;
            for (size_t k = j + 1, d = 0; k < close; k++) {
                if (punct(a, k, "(") || punct(a, k, "[") || punct(a, k, "{")) d++;
                else if (punct(a, k, ")") || punct(a, k, "]") || punct(a, k, "}")) d--;
                else if (d == 0 && punct(a, k, ",")) count++;
                else if (d == 0 && punct(a, k, "..")) return -1;            /* the view: not typed */
            }
            t = index_type(c, t, count);
            j = close + 1;
        } else if ((punct(a, j, ".") || punct(a, j, "->")) && ident(a, j + 1)) {
            t = field_type(c, t, tok(a, j + 1));
            j += 2;
        } else {
            return -1;
        }
    }
    return j == r.end ? t : -1;
}

static void resolution_begin(Ctx *c) { if (c->nest++ == 0) c->type_count = 0; }
static void resolution_end(Ctx *c) { c->nest--; }

/* The instance a call's verb belongs to is named by the module and the
   arguments: `keel_routine_ag2_Ag` for `seq`, where the receiver is a table of
   slots and the modifier is not the first parameter. A verb whose object is the
   modifier itself takes the type's own symbol instead: `keel_routine_slot_ag2_Ag_code`. */
static bool module_instance_prefix(Ctx *c, const KAst *home, int inst, Text *t) {
    put_module_prefix(t, home);
    for (int i = 0; inst >= 0 && i < kt(c, inst)->argc; i++) {
        const char *s = kt_symbol(c, kt(c, inst)->arg[i]);
        if (!*s) return false;
        put_str(t, "_");
        put_str(t, s);
    }
    return t->ok;
}

static bool mentions_own(const KAst *home, const Param *p) {
    keel_slice_char own = modifier_of(home);
    for (size_t k = p->first; own.len && k < p->end; k++)
        if (ident(home, k) && k_symtab_same_name(tok(home, k), own)) return true;
    return false;
}

static int prim_type(Ctx *c, const char *name) {
    int t = kt_new(c);
    if (t < 0) return -1;
    kt(c, t)->kind = KT_PRIM;
    kt_set_symbol(kt(c, t), name, strlen(name));
    return t;
}

/* One call: `callee` is the first token of the written callee, `verb` its
   last, `open` the '(' — `module` is the qualifier's module, NULL for a call
   to the file's own function. The island is always printed, resolved or not:
   what the module does not declare goes on as the qualified call, for the C
   compiler to validate (spec §4.4, item 3). It is not printed only where this
   pass cannot name the instance: the object is a shape it does not type yet.
   Returns the type of the result, or -1. */
static int call_core(Ctx *c, size_t callee, size_t verb, size_t open, const KModule *module) {
    KAst *a = c->ast;
    size_t close = close_paren(a, open, a->tokens.len);
    if (close == SIZE_MAX) return -1;

    Range args[K_PARAMS_MAX];
    size_t argc = 0, start = open + 1, depth = 0;
    for (size_t j = open + 1; j <= close; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (j < close && (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}"))) depth--;
        else if (j == close || (depth == 0 && punct(a, j, ","))) {
            if (j > start || argc > 0) {
                if (argc == K_PARAMS_MAX) return -1;
                args[argc++] = (Range){ start, j };
            }
            start = j + 1;
        }
    }

    const KAst *home = module ? ast_of(c, module) : a;
    if (!home) return -1;
    KToken name = tok(a, verb);
    keel_slice_char written = range_text(a, (Range){ callee, verb + 1 });
    int t0 = argc ? type_of(c, args[0]) : -1;
    const KType *k0 = t0 >= 0 ? kt(c, t0) : NULL;
    keel_slice_char none = {0};

    /* the core's `length`, `capacity` and `dim` lower to no function (backend §5.2):
       the island says `core`, and the dimension of the array comes from the table */
    if (module && module_named(home, "keel")) {
        bool length = k_token_spelled(name, "length"), capacity = k_token_spelled(name, "capacity"),
             dim = k_token_spelled(name, "dim");
        if (!(length || capacity || dim) || !argc) return -1;
        if (!k0 || k0->kind != KT_ARRAY) {
            if ((k0 && k0->kind != KT_NONE) || container_shape(a, args[0]) == SHAPE_C)
                diag3(c, K_DIAG_NOT_A_CONTAINER_EXPRESSION, tok(a, args[0].first), written, range_text(a, args[0]), none);
            return -1;
        }
        long known;
        if (dim && argc == 2 && !decimal_of(c, args[1], &known))
            diag3(c, K_DIAG_NONCONSTANT_DIM_INDEX, tok(a, args[1].first), range_text(a, args[1]), none, none);
        Text t = { .ok = true };
        put(&t, written.ptr, written.len);
        put_str(&t, "/");
        put_uint(&t, argc);
        put_str(&t, " \xe2\x86\x92 core dim:1");
        emit(c, K_ISLAND_CALL, callee, &t);
        return prim_type(c, "size_t");
    }

    keel_slice_char redirected = {0};
    /* A function over a protocol (spec §4.14, backend §5.19): the instance is
       named by the concrete type of the argument, `<prefix>_<name><arity>_<type>`.
       The island resolves the call as the concrete type's own verb — the one
       whose result is the function's associated return type, or the first verb
       that takes the receiver — to find the instance, the adaptation and the
       type of the result. Only the first parameter may be the protocol one. */
    bool proto_of = false;
    size_t redirect_arity = argc;
    const KAst *proto_home = NULL;
    Sig fsig = {0};
    if (argc) {
        Sig f = find_verb(home, name, argc);
        if (f.found && f.n && f.p[0].is_protocol) {
            KProtocolDecl pd;
            if (!protocol_decl(c, home, f.p[0].base, &pd, 0)) return -1;
            const KModule *own_module = NULL;
            if (k0 && k0->kind == KT_ARRAY) {
                if (k0->rank > 1) {
                    diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, tok(a, callee), written, range_text(a, args[0]), k0->dims);
                    return -1;
                }
                own_module = module_by_name(a->symbols, "keel.array");
            } else if (k0 && (k0->kind == KT_MODIFIER || k0->kind == KT_NAMED)) {
                own_module = k0->module;
            } else {
                return -1;
            }
            const KAst *own = own_module && own_module->ast ? own_module->ast
                            : k0->kind == KT_ARRAY ? NULL : a;   /* a type of this very file */
            Text missing = { .ok = true };
            if (!own || !satisfies(c, own, k0->kind == KT_ARRAY ? (keel_slice_char){0} : k0->name, &pd, &missing, tok(a, callee))) {
                diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, callee), range_text(a, args[0]), f.p[0].base,
                      own ? keep(c, &missing) : k_diag_text("its verbs"));
                return -1;
            }
            const KProtocolVerb *v = verb_for_return(home, f.fn, &pd);
            if (!v) return -1;
            proto_of = true;
            proto_home = home;
            fsig = f;
            module = own_module;
            home = own;
            redirected = v->name;
            redirect_arity = v->arity;
        }
    }
    Sig sig = find_verb(home, redirected.len ? redirected : name, redirected.len ? redirect_arity : argc);
    keel_slice_char declared = redirected.len ? redirected : name;
    bool gen = module && generic(home);
    keel_slice_char modifier = gen ? modifier_of(home) : none;

    /* which parameter takes the container, which one mentions the module's modifier
       inside another type, which takes an `array`, and whether a `type` parameter
       selects the instance (spec §4.4, item 4) */
    size_t recv = SIZE_MAX, nested = SIZE_MAX, arr = SIZE_MAX;
    bool selection = false;
    for (size_t k = 0; sig.found && k < sig.n; k++) {
        if (sig.p[k].is_type) selection = selection || binder(home, sig.p[k].base);
        else if (sig.p[k].is_array) { if (arr == SIZE_MAX) arr = k; }
        else if (gen && recv == SIZE_MAX && k_symtab_same_name(sig.p[k].base, modifier)) recv = k;
        else if (gen && nested == SIZE_MAX && mentions_own(home, &sig.p[k])) nested = k;
    }
    if (!sig.found && gen && argc) recv = 0;        /* the module declares no such verb: the object is first */

    /* slice.from over a `ref` would take a pointer keel does not vouch for (spec §5.3) */
    if (module && module_named(home, "keel.slice") && k_token_spelled(name, "from") && argc >= 2) {
        const Local *p = arg_local(c, args[1]);
        if (p && p->is_ref)
            diag3(c, K_DIAG_SLICE_FROM_REF, tok(a, args[1].first), range_text(a, args[1]), none, none);
    }

    Text t = { .ok = true };
    put(&t, written.ptr, written.len);
    put_str(&t, "/");
    put_uint(&t, argc);
    put_str(&t, " \xe2\x86\x92 ");

    /* buffer.of(x) needs an `array` of one dimension, to read the extent from (spec §5.3) */
    if (module && !redirected.len && module_named(home, "keel.buffer") && k_token_spelled(name, "of") &&
        argc == 1 && k0 && k0->kind != KT_ARRAY && k0->kind != KT_NONE) {
        diag3(c, K_DIAG_BUFFER_OF_UNKNOWN_SIZE, tok(a, callee), written, range_text(a, args[0]), none);
        return -1;
    }

    /* arena.from_array(a, v) takes an `array u8` by its binder (spec §5.2); the
       element type is the C compiler's to check */
    if (module && module_named(home, "keel.arena") && k_token_spelled(name, "from_array") && argc == 2) {
        int t1 = type_of(c, args[1]);
        if (t1 < 0 || kt(c, t1)->kind != KT_ARRAY) return -1;
        Text array_form = { .ok = true };
        put(&array_form, written.ptr, written.len);
        put_str(&array_form, "/2 \xe2\x86\x92 ");
        put_module_prefix(&array_form, home);
        put_str(&array_form, "_from_array");
        if (k0 && k0->pointers == 0 && k0->lvalue) put_str(&array_form, " &1");
        put_str(&array_form, " dim:2");
        emit(c, K_ISLAND_CALL, callee, &array_form);
        return prim_type(c, "bool");
    }

    /* `buffer.of(v)` over an `array` lowers to the buffer's `of`, with dimension 0
       from the table (backend §5.2) */
    if (gen && !redirected.len && k0 && k0->kind == KT_ARRAY && k_token_spelled(name, "of") &&
        module_named(home, "keel.buffer")) {
        const char *elem = kt_symbol(c, k0->elem);
        if (!*elem) return -1;
        if (k0->rank > 1) {
            diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, tok(a, callee), written, range_text(a, args[0]), k0->dims);
            return -1;
        }
        if (argc == 1 && !strncmp(elem, "const_", 6))
            diag3(c, K_DIAG_BUFFER_OVER_CONST, tok(a, callee), written, (keel_slice_char){ strlen(elem), (char *)elem }, none);
        if (argc != 1) return -1;                   /* buffer.of(p, n) is the ordinary verb */
        put_module_prefix(&t, home);
        put_str(&t, "_");
        put_str(&t, elem);
        put_str(&t, "_of dim:1");
        emit(c, K_ISLAND_CALL, callee, &t);
        /* the result is a buffer over the element */
        int view = kt_new(c);
        if (view < 0) return -1;
        kt(c, view)->kind = KT_MODIFIER;
        kt(c, view)->module = module;
        kt(c, view)->argc = 1;
        kt(c, view)->arg[0] = kt_copy(c, k0->elem);
        char symbol[K_SYMBOL_MAX];
        Text sy = { .ok = true };
        put_module_prefix(&sy, home);
        put_str(&sy, "_");
        put_str(&sy, elem);
        if (sy.ok) { memcpy(symbol, sy.s, sy.n); kt_set_symbol(kt(c, view), symbol, sy.n); }
        return view;
    }

    int inst = -1;                                  /* the instance of the module the verb belongs to */
    bool direct = false;                            /* the object is the modifier itself */
    if (gen) {
        if (recv != SIZE_MAX || nested != SIZE_MAX || arr != SIZE_MAX) {
            size_t k = recv != SIZE_MAX ? recv : nested != SIZE_MAX ? nested : arr;
            if (k >= argc) return -1;
            Range r = args[k];
            if (punct(a, r.first, "&") && recv != SIZE_MAX) {
                diag3(c, K_DIAG_ADDRESS_IN_OBJECT_POSITION, tok(a, r.first), written, range_text(a, r), none);
                return -1;
            }
            int tk = k == 0 ? t0 : type_of(c, r);
            const KType *ok = tk >= 0 ? kt(c, tk) : NULL;
            bool wanted = ok && (arr != SIZE_MAX && recv == SIZE_MAX && nested == SIZE_MAX ? ok->kind == KT_ARRAY
                                                                                            : ok->kind == KT_MODIFIER);
            if (!wanted) {
                /* only what is certainly not a container is refused: a name seen
                   declared as something else, or a shape that is C. A name this pass
                   could not have seen declared is not refused. */
                if ((ok && ok->kind != KT_NONE) || container_shape(a, r) == SHAPE_C)
                    diag3(c, K_DIAG_NOT_A_CONTAINER_EXPRESSION, tok(a, r.first), written, range_text(a, r), none);
                return -1;
            }
            if (recv != SIZE_MAX) {
                if (ok->module != module) {
                    if (ok->module && !sig.producer)
                        diag3(c, K_DIAG_WRONG_QUALIFIER, tok(a, callee), written, alias_of(a->symbols, ok->module),
                              range_text(a, r));
                    return -1;
                }
                inst = tk;
                direct = true;
                /* get and set copy an element: an instance of a modifier is not copied by value */
                if ((k_token_spelled(name, "get") || k_token_spelled(name, "set")) && ok->argc &&
                    ok->arg[0] >= 0 && kt(c, ok->arg[0])->kind == KT_MODIFIER)
                    diag3(c, K_DIAG_ELEMENT_COPY_IN_GET_SET, tok(a, callee), written,
                          (keel_slice_char){ strlen(kt_symbol(c, ok->arg[0])), (char *)kt_symbol(c, ok->arg[0]) }, none);
            } else if (nested != SIZE_MAX) {
                inst = kt_find(c, tk, module, 0);
                if (inst < 0) return -1;
            } else {
                /* the verbs of `keel.array` are the family of one element type */
                inst = kt_new(c);
                if (inst < 0) return -1;
                kt(c, inst)->kind = KT_MODIFIER;
                kt(c, inst)->module = module;
                kt(c, inst)->argc = 1;
                kt(c, inst)->arg[0] = kt_copy(c, ok->elem);
            }
        } else if (selection) {
            /* the written types are the instance's arguments, in parameter order */
            Text spelled = { .ok = true };
            put(&spelled, modifier.ptr, modifier.len);
            for (size_t k = 0; k < sig.n && k < argc; k++) {
                if (!sig.p[k].is_type || !binder(home, sig.p[k].base)) continue;
                put_str(&spelled, " ");
                put_tokens(&spelled, a, args[k].first, args[k].end);
            }
            if (!spelled.ok) return -1;
            inst = kt_from_text(c, a->symbols, (keel_slice_char){ spelled.n, spelled.s });
        } else if (sig.found && c->has_target && c->target.kind == K_SPEC_MODIFIER) {
            inst = kt_from_text(c, a->symbols, c->target.text);
            if (inst < 0 || kt(c, inst)->module != module) return -1;
        } else {
            /* from-without-target waits for the targets of an assignment and of
               a return: refusing now would refuse valid programs */
            return -1;
        }
        if (inst < 0 || kt(c, inst)->kind != KT_MODIFIER) return -1;
        if (proto_of) {
            /* keel_slice_of<arity>_<concrete type> (backend §5.19): the concrete
               type's canonical name is the prefix its own verbs carry */
            Text own = { .ok = true };
            if (direct ? (put_str(&own, kt(c, inst)->symbol), !*kt(c, inst)->symbol)
                       : !module_instance_prefix(c, home, inst, &own)) return -1;
            put_module_prefix(&t, proto_home);
            put_str(&t, "_");
            put(&t, name.ptr, name.len);
            if (fsig.overloaded && argc > 1) put_uint(&t, argc - 1);
            put_str(&t, "_");
            put(&t, own.s, own.n);
        } else if (direct) {
            if (!*kt(c, inst)->symbol) return -1;
            put_str(&t, kt(c, inst)->symbol);
        } else if (!module_instance_prefix(c, home, inst, &t)) {
            return -1;
        }
        /* a verb the instance does not keep (spec §4.3, rule 13) */
        const char *cause = NULL;
        if (!proto_of && sig.found && !c->quiet && !verb_available(c, home, sig.fn, inst, &cause)) {
            Text written = { .ok = true };
            put(&written, tok(a, callee).ptr, (size_t)(tok(a, verb).ptr + tok(a, verb).len - tok(a, callee).ptr));
            Text where = { .ok = true }, why = { .ok = true };
            put_str(&where, kt(c, inst)->symbol);
            put_str(&why, cause);
            diag3(c, K_DIAG_VERB_NOT_IN_INSTANCE, tok(a, callee), keep(c, &written), keep(c, &where), keep(c, &why));
        }
    } else if (proto_of) {
        /* a type of a module that is not generic: its canonical name is its symbol */
        if (!k0 || !*k0->symbol) return -1;
        put_module_prefix(&t, proto_home);
        put_str(&t, "_");
        put(&t, name.ptr, name.len);
        if (fsig.overloaded && argc > 1) put_uint(&t, argc - 1);
        put_str(&t, "_");
        put_str(&t, k0->symbol);
    } else {
        put_module_prefix(&t, home);
    }
    if (!proto_of) {
        put_str(&t, "_");
        put(&t, declared.ptr, declared.len);
        if (sig.found && sig.overloaded && argc > 1) put_uint(&t, argc - 1);
    }

    /* a value of a set where the verb's parameter is the module's `tags`
       binder belongs to the instance's set (spec §4.3, rule 6) */
    if (inst >= 0 && sig.found && !proto_of && kt(c, inst)->kind == KT_MODIFIER) {
        const KAstNode *mn = module_node(home);
        size_t bi = 0;
        for (size_t j = mn->tags_first; j < mn->tags_end; j++) {
            if (!ident(home, j)) continue;
            for (size_t k = 0; k < argc && k < sig.n; k++) {
                if (!k_symtab_same_name(sig.p[k].base, tok(home, j)) || sig.p[k].pointer) continue;
                if (args[k].end != args[k].first + 1 || !ident(a, args[k].first) || (int)bi >= kt(c, inst)->argc) continue;
                const KType *set = kt(c, kt(c, inst)->arg[bi]);
                const KAst *where = set->module && ast_of(c, set->module) ? ast_of(c, set->module) : a;
                KTagsDecl tags;
                if (!tags_decl(where, set->name, &tags)) continue;
                bool in = false;
                for (size_t m = 0; m < tags.item_count; m++) if (k_symtab_same_name(tags.items[m], tok(a, args[k].first))) in = true;
                if (!in && tag_of_some_set(a, tok(a, args[k].first)))
                    diag3(c, K_DIAG_TAG_FROM_OTHER_SET, tok(a, args[k].first), tok(a, args[k].first), set->name,
                          (keel_slice_char){0});
            }
            bi++;
        }
    }

    /* adaptation marks, from the declared parameter (spec §4.4, item 5) */
    for (size_t k = 0; sig.found && k < argc && k < sig.n; k++) {
        if (sig.p[k].is_type) {
            if (!binder(home, sig.p[k].base)) { put_str(&t, " type:"); put_uint(&t, k + 1); }
            continue;
        }
        int tk = k == 0 ? t0 : type_of(c, args[k]);
        const KType *ak = tk >= 0 ? kt(c, tk) : NULL;
        if (sig.p[k].is_array) {
            if (ak && ak->kind == KT_ARRAY) { put_str(&t, " dim:"); put_uint(&t, k + 1); }
            continue;
        }
        if (sig.p[k].pointer && ak && ak->pointers == 0 && ak->lvalue) { put_str(&t, " &"); put_uint(&t, k + 1); }
    }
    emit(c, K_ISLAND_CALL, callee, &t);
    if (proto_of && !returns_associated(proto_home, fsig.fn))
        return ret_type(c, proto_home, fsig.fn, -1);
    return sig.found && sig.fn ? ret_type(c, home, sig.fn, inst) : -1;
}

/* a call at token `i`, if it is one */
static bool call_at(Ctx *c, size_t i) {
    Callee ce;
    resolution_begin(c);
    bool is = callee_at(c, i, &ce);
    if (is) call_core(c, i, ce.verb, ce.open, ce.module);
    resolution_end(c);
    return is;
}

/* ---- names ------------------------------------------------------------- */

/* the tags item `name`, if some `tags` declaration of the file lists it: its symbol */
static bool tag_value(const KAst *a, keel_slice_char name, Text *t) {
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = node_at(a, i);
        if (n->kind != K_AST_TAGS) continue;
        size_t j = n->name_end;
        if (!punct(a, j, "[")) continue;
        for (bool item = true; j < n->end && !punct(a, j, "]"); j++) {
            if (punct(a, j, "[") || punct(a, j, ",")) item = true;
            else if (item && ident(a, j)) {
                item = false;
                if (!k_symtab_same_name(tok(a, j), name)) continue;
                put_module_prefix(t, a);
                put_str(t, "_");
                put_glued(t, a, n->name_first, n->name_end);
                put_str(t, "_");
                put(t, name.ptr, name.len);
                return true;
            }
        }
    }
    return false;
}

/* whether `name` is a value of a set declared in `where` */
static bool tag_of_some_set(const KAst *where, keel_slice_char name) {
    Text scratch = { .ok = true };
    return tag_value(where, name, &scratch);
}

static void name_island(Ctx *c, size_t i) {
    KAst *a = c->ast;
    KToken name = tok(a, i);
    const Local *l = find_local(c, name);
    if (l && !l->global) return;                    /* a local, or a parameter: not a name of the module */
    Text t = { .ok = true };
    put(&t, name.ptr, name.len);
    put_str(&t, " \xe2\x86\x92 ");
    const KSymbol *s = k_symbol_resolve(a->symbols, name);
    if (s && (s->kind == K_SYM_CONSTANT || s->kind == K_SYM_VARIABLE || s->kind == K_SYM_FUNCTION) && s->origin) {
        char sym[K_DETAIL_MAX];
        size_t n = k_mangle_symbol(s, (keel_slice_char){ sizeof sym, sym });
        if (n > sizeof sym) return;
        put(&t, sym, n);
    } else if (!tag_value(a, name, &t)) {
        return;
    }
    emit(c, K_ISLAND_NAME, i, &t);
}

/* `Q.IDENT` that is not a call: a constant, a variable or a function of the
   module the qualifier names. What the module does not export goes on as the
   qualified name, for the C compiler to validate (spec §4.4, item 3). */
static void qualified_name(Ctx *c, size_t i, const KModule *module) {
    KAst *a = c->ast;
    KToken member = tok(a, i + 2);
    Text t = { .ok = true };
    put_glued(&t, a, i, i + 3);
    put_str(&t, " \xe2\x86\x92 ");
    const KSymbol *found = NULL;
    for (size_t k = 0; k < module->symbol_count && !found; k++)
        if (k_symtab_same_name(module->symbols[k].name, member)) found = &module->symbols[k];
    if (found) {
        char sym[K_DETAIL_MAX];
        size_t n = k_mangle_symbol(found, (keel_slice_char){ sizeof sym, sym });
        if (n > sizeof sym) return;
        put(&t, sym, n);
    } else if (module->ast) {
        put_module_prefix(&t, module->ast);
        put_str(&t, "_");
        put(&t, member.ptr, member.len);
    } else {
        return;
    }
    emit(c, K_ISLAND_NAME, i, &t);
}

/* ---- declarations ------------------------------------------------------ */

static bool is_arena(const KAst *a, const KSpecifier *spec) {
    if (spec->kind != K_SPEC_NAMED_TYPE) return false;
    const KSymbol *s = k_symbol_resolve(a->symbols, spec->type_name);
    return s && s->kind == K_SYM_TYPE && s->origin &&
           k_symtab_same_name(s->origin->name, k_diag_text("keel.arena")) &&
           k_symtab_same_name(s->name, k_diag_text("arena"));
}

/* A keel type at token `i`: its `type` island, then what it declares. Returns
   the index of the last token of the specifier. */
static void check_arguments(Ctx *c, const KSpecifier *spec, KToken at);
static void check_name(Ctx *c, KToken name, bool top);

static void check_constexpr(Ctx *c, size_t kw, size_t end);
static bool spec_byref(const KAst *a, const KSpecifier *spec);

static size_t declaration(Ctx *c, size_t i, const KSpecifier *spec, KLexer *lexer, KToken next,
                          bool signature, bool declarators) {
    KAst *a = c->ast;
    check_arguments(c, spec, tok(a, i));
    size_t end = index_at(a, i, spec->text.ptr + spec->text.len);

    bool primitive = spec->kind == K_SPEC_NAMED_TYPE && k_primitive_name(spec->type_name);
    if (!primitive) {
        char sym[K_DETAIL_MAX];
        size_t n = SIZE_MAX;
        if (spec->kind == K_SPEC_MODIFIER) n = k_spec_symbol(spec->text, a->symbols, sym, sizeof sym);
        else {
            const KSymbol *s = k_symbol_resolve(a->symbols, spec->type_name);
            if (s && s->origin) n = k_mangle_symbol(s, (keel_slice_char){ sizeof sym, sym });
        }
        if (n <= sizeof sym) {
            Text t = { .ok = true };
            put_tokens(&t, a, i, end);
            put_str(&t, " \xe2\x86\x92 ");
            put(&t, sym, n);
            emit(c, K_ISLAND_TYPE, i, &t);
        }
    }

    if (!declarators) return end - 1;

    bool external = i > 0 && word(a, i - 1, "extern");
    TKPpKind pp;
    for (KToken d = next; d.len; ) {
        KDeclarator decl;
        KToken after;
        if (!(k_token_is_punct(d, "*") || k_token_is_punct(d, "(") || k_token_is_ident(d))) break;
        if (!k_scan_declarator(lexer, d, &decl, &after, &pp) || !decl.name.len) break;
        if (decl.has_function_suffix && !decl.parenthesized) break;   /* a prototype */
        size_t at = index_at(a, i, decl.name.ptr);
        if (!signature && c->depth > 0) check_name(c, decl.name, false);
        add_local(c, (Local){ .name = decl.name, .spec = *spec, .decl_at = at,
                              .pointers = decl.pointer_depth + (decl.has_array_suffix ? 1 : 0) });

        bool initialized = k_token_is_punct(after, "=");
        if (!signature && !external && !initialized && decl.pointer_depth == 0 &&
            !decl.parenthesized && is_arena(a, spec) &&
            (k_token_is_punct(after, ",") || k_token_is_punct(after, ";"))) {
            Text t = { .ok = true };
            put(&t, decl.name.ptr, decl.name.len);
            emit(c, K_ISLAND_IMPLICIT_INIT, at, &t);
        }
        if (initialized) {
            static const char *const stops[] = { ",", ";" };
            size_t which;
            if (!c->has_target) { c->has_target = true; c->target = *spec; }
            KToken value = k_lexer_next(lexer, &pp);
            k_scan_opaque_until(lexer, value, stops, 2, &which, &after, &pp);
        }
        if (signature || !k_token_is_punct(after, ",")) break;
        d = k_lexer_next(lexer, &pp);
    }
    return end - 1;
}

/* ---- stage 4b: array, array-index, index, range-index ------------------ */

/* decl-array (spec §2.2, §4.2) at the `array` word `i`: one island per declared
   name, with the dimensions as written, and the name known as an array from
   there on. Returns the last token of the element type, which is not a
   declaration of its own: the caller skips to it. */
static size_t array_decl(Ctx *c, size_t i, bool signature) {
    KAst *a = c->ast;
    if (i + 1 >= a->tokens.len) return i;
    KLexer lexer;
    TKPpKind pp;
    KToken next = {0}, first = enter(a, i + 1, &lexer, &pp);
    keel_slice_char element;
    if (!k_scan_argument(&lexer, first, a->symbols, &element, &next, &pp)) return i;
    size_t last = index_at(a, i + 1, next.len ? next.ptr : a->source.ptr + a->source.len) - 1;

    /* a keel element type is a use like any other; the declarators follow it */
    KLexer look;
    KSpecifier spec;
    KToken after_type = {0}, head = enter(a, i + 1, &look, &pp);
    if (k_token_is_ident(head) && k_scan_known_type(&look, head, a->symbols, &spec, &after_type, &pp) &&
        spec.kind != K_SPEC_NONE)
        declaration(c, i + 1, &spec, &look, after_type, signature, false);

    const char *source_end = a->source.ptr + a->source.len;
    for (KToken d = next; d.len; ) {
        KDeclarator decl;
        KToken after;
        if (!(k_token_is_punct(d, "*") || k_token_is_ident(d))) break;
        if (!k_scan_declarator(&lexer, d, &decl, &after, &pp) || !decl.name.len ||
            !decl.has_array_suffix || decl.parenthesized) break;
        const char *dims = decl.name.ptr + decl.name.len, *end = after.len ? after.ptr : source_end;
        Text t = { .ok = true };
        put(&t, decl.name.ptr, decl.name.len);
        put_str(&t, " ");
        put_source(&t, dims, end);
        emit(c, K_ISLAND_ARRAY, i, &t);
        if (signature) {
            /* a parameter names its dimension 0 with a binder, or is not one-dimensional (spec §4.2, item 19) */
            const char *p = dims, *q = end;
            while (p < q && (*p == ' ' || *p == '[')) p++;
            bool empty = p < q && *p == ']';
            bool binder_written = q - p >= 6 && !memcmp(p, "size_t", 6);
            if (empty)
                diag3(c, K_DIAG_ARRAY_PARAMETER_WITHOUT_DIMENSION, decl.name, decl.name, (keel_slice_char){0},
                      (keel_slice_char){0});
            else if (rank_of(dims, end) == 1 && !binder_written)
                diag3(c, K_DIAG_ARRAY_1D_AS_PARAMETER, decl.name, decl.name, (keel_slice_char){ (size_t)(end - dims), (char *)dims },
                      (keel_slice_char){0});
        }
        add_local(c, (Local){ .name = decl.name, .pointers = decl.pointer_depth + 1,
                              .decl_at = index_at(a, i, decl.name.ptr), .rank = rank_of(dims, end),
                              .element = element, .dims = { (size_t)(end - dims), (char *)dims } });
        if (k_token_is_punct(after, "=")) {
            static const char *const stops[] = { ",", ";" };
            size_t which;
            KToken value = k_lexer_next(&lexer, &pp);
            k_scan_opaque_until(&lexer, value, stops, 2, &which, &after, &pp);
        }
        if (signature || !k_token_is_punct(after, ",")) break;
        d = k_lexer_next(&lexer, &pp);
    }
    return last;
}

/* `x[a..b]` with both limits known decimals and a start above the end */
static void check_inverted(Ctx *c, KToken name, size_t open, size_t range, size_t close) {
    const KAst *a = c->ast;
    long low, high;
    if (range > open + 1 && range + 1 < close &&
        decimal_of(c, (Range){ open + 1, range }, &low) && decimal_of(c, (Range){ range + 1, close }, &high) &&
        low > high)
        diag3(c, K_DIAG_INVERTED_RANGE_INDEX, name,
              (keel_slice_char){ (size_t)(tok(a, close).ptr - tok(a, open).ptr) - 1, tok(a, open).ptr + 1 },
              (keel_slice_char){0}, (keel_slice_char){0});
}

/* `<type symbol>_<verb><suffix>` for a container and a call of `arity` arguments,
   object included; `adapt` says the object takes an `&`. */
static bool container_verb(const KType *k, const KAst *home, keel_slice_char verb, size_t arity, Text *tx,
                           bool *adapt) {
    Sig sig = find_verb(home, verb, arity);
    if (!sig.found || !*k->symbol) return false;
    put_str(tx, k->symbol);
    put_str(tx, "_");
    put(tx, verb.ptr, verb.len);
    if (sig.overloaded && arity > 1) put_uint(tx, arity - 1);
    if (sig.n && sig.p[0].pointer && k->pointers == 0 && k->lvalue) *adapt = true;
    return true;
}

/* One bracket group after the path [i, open): the islands of `x[…]`, where the
   container is any expression this pass can type (spec §2.2). An `array` takes
   its indices as they come, in one bracket or several; a modifier's container
   goes through `ptr`, or the verb of range-index. Returns the token after what
   it consumed. */
static size_t index_group(Ctx *c, size_t i, size_t open, bool *any) {
    KAst *a = c->ast;
    size_t close = close_bracket(a, open, a->tokens.len);
    if (close == SIZE_MAX) return a->tokens.len;
    KToken name = tok(a, i);
    int t = type_of(c, (Range){ i, open });
    if (t < 0) return close + 1;
    const KType *k = kt(c, t);
    keel_slice_char path = { (size_t)(tok(a, open - 1).ptr + tok(a, open - 1).len - name.ptr), name.ptr };
    keel_slice_char none = {0};

    size_t commas = 0, range = SIZE_MAX, depth = 0;
    for (size_t j = open + 1; j < close; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) depth--;
        else if (depth == 0 && punct(a, j, ",")) commas++;
        else if (depth == 0 && punct(a, j, "..") && range == SIZE_MAX) range = j;
    }

    /* an open end reads the container twice (spec §4.5, item 9): the path before
       it may not hold an index — `x[i][a..]` — for the read would repeat it */
    for (size_t o = close + 1; range == SIZE_MAX && punct(a, o, "["); ) {
        size_t next = close_bracket(a, o, a->tokens.len);
        if (next == SIZE_MAX) break;
        for (size_t j = o + 1, d = 0; j < next; j++) {
            if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) d++;
            else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) d--;
            else if (d == 0 && punct(a, j, "..") && j > o + 1 && j + 1 == next)
                diag3(c, K_DIAG_OPEN_RANGE_INDEX_ON_COMPLEX_PATH, name,
                      (keel_slice_char){ (size_t)(tok(a, next).ptr - tok(a, o).ptr) + 1, tok(a, o).ptr }, none, none);
        }
        o = next + 1;
    }

    Text tx = { .ok = true };
    if (k->kind == KT_ARRAY) {
        if (range != SIZE_MAX) {
            /* `x[a..b]` over an `array` is the `as_slice` of `keel.array`: the element
               gives the instance, and dimension 0 comes from the table (spec §4.5) */
            check_inverted(c, name, open, range, close);
            const char *elem = kt_symbol(c, k->elem);
            if (commas || !*elem) return close + 1;
            keel_slice_char group = { (size_t)(tok(a, close).ptr - tok(a, open).ptr) + 1, tok(a, open).ptr };
            if (k->rank > 1) {
                diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, name, k_diag_text("as_slice"), group, k->dims);
                return close + 1;
            }
            const KModule *am = module_by_name(a->symbols, "keel.array");
            Sig sig = am && ast_of(c, am) ? find_verb(ast_of(c, am), k_diag_text("as_slice"), 3) : (Sig){0};
            if (!sig.found) {
                diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, name, path, k_diag_text("Sliceable"), k_diag_text("length, as_slice"));
                return close + 1;
            }
            bool high = range + 1 < close;
            put(&tx, path.ptr, path.len);
            put_str(&tx, " ");
            put_source(&tx, tok(a, open).ptr + 1, tok(a, close).ptr);
            put_str(&tx, " \xe2\x86\x92 ");
            put_module_prefix(&tx, ast_of(c, am));
            put_str(&tx, "_");
            put_str(&tx, elem);
            put_str(&tx, sig.overloaded ? "_as_slice2" : "_as_slice");
            if (!high) put_str(&tx, " core");         /* the limit is keel.length(x) */
            put_str(&tx, " dim:1");
            *any = emit(c, K_ISLAND_RANGE_INDEX, i, &tx) || *any;
            return close + 1;
        }
        Range idx[8];
        size_t indices = 0, last = close;
        for (size_t o = open; punct(a, o, "[") && indices < (size_t)k->rank; ) {
            size_t cl = close_bracket(a, o, a->tokens.len);
            if (cl == SIZE_MAX) break;
            size_t st = o + 1, d = 0;
            for (size_t m = o + 1; m <= cl; m++) {
                if (punct(a, m, "(") || punct(a, m, "[") || punct(a, m, "{")) d++;
                else if (m < cl && (punct(a, m, ")") || punct(a, m, "]") || punct(a, m, "}"))) d--;
                else if (m == cl || (d == 0 && punct(a, m, ","))) {
                    if (indices < 8) idx[indices] = (Range){ st, m };
                    indices++;
                    st = m + 1;
                }
            }
            last = cl;
            o = cl + 1;
        }
        put(&tx, path.ptr, path.len);
        put_str(&tx, " rank ");
        put_uint(&tx, indices);
        if (indices < (size_t)k->rank) diag3(c, K_DIAG_PARTIAL_ARRAY_INDEX, name, k->dims, none, none);
        long dims[8];
        size_t known = dims_of(c, k->dims, dims);
        for (size_t n = 0; n < indices && n < known && n < 8; n++) {
            long v;
            if (dims[n] >= 0 && decimal_of(c, idx[n], &v) && v >= dims[n])
                diag3(c, K_DIAG_ARRAY_INDEX_ABOVE_DIMENSION, tok(a, idx[n].first), range_text(a, idx[n]), k->dims, none);
        }
        *any = emit(c, K_ISLAND_ARRAY_INDEX, i, &tx) || *any;
        return last + 1;
    }

    if (k->kind != KT_MODIFIER || !k->module) return close + 1;
    const KAst *home = ast_of(c, k->module);
    if (!home || close == open + 1) return close + 1;
    if (module_node(home)->dim_first != module_node(home)->dim_end) return close + 1;   /* full-rank accessor: §5.3.1 */
    keel_slice_char text = { (size_t)(tok(a, close).ptr - tok(a, open).ptr) + 1, tok(a, open).ptr };
    bool adapt = false;
    put(&tx, path.ptr, path.len);
    if (range == SIZE_MAX) {
        /* x[i] is *ptr(x, i); x[i, j] is *ptr(x, idx) with a vector of indices, and a
           module with no `dim` declares none (spec §4.5, item 1) */
        put_str(&tx, " \xe2\x86\x92 ");
        if (commas || !container_verb(k, home, k_diag_text("ptr"), 2, &tx, &adapt)) {
            diag3(c, K_DIAG_NO_PTR_FOR_ARITY, name, path, text, none);
            return close + 1;
        }
    } else {
        /* x[a..b] is as_slice(x, a, b) of the module of the container's type, and the
           open ends read `length` (spec §4.5, items 5 and 6) */
        if (commas) return close + 1;
        check_inverted(c, name, open, range, close);
        bool high = range + 1 < close;
        put_str(&tx, " ");
        put_source(&tx, tok(a, open).ptr + 1, tok(a, close).ptr);
        put_str(&tx, " \xe2\x86\x92 ");
        bool ignored = false;
        if (!container_verb(k, home, k_diag_text("as_slice"), 3, &tx, &adapt) ||
            (!high && (put_str(&tx, " "), !container_verb(k, home, k_diag_text("length"), 1, &tx, &ignored)))) {
            diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, name, path, k_diag_text("Sliceable"), k_diag_text("length, as_slice"));
            return close + 1;
        }
    }
    if (adapt) put_str(&tx, " &1");
    *any = emit(c, range == SIZE_MAX ? K_ISLAND_INDEX : K_ISLAND_RANGE_INDEX, i, &tx) || *any;
    return close + 1;
}

static size_t column_access(Ctx *c, size_t i, size_t open, bool *any);

/* `x[`, `x.f[`, `x->f[i][j]`: every index or range-index over a container this
   pass can type is an island, even when the emission is the written text (spec
   §2.3). True when one was printed. */
static bool path_forms(Ctx *c, size_t i) {
    KAst *a = c->ast;
    const Local *head = find_local(c, tok(a, i));
    if (!head || head->decl_at == i) return false;      /* not ours, or the declarator's own brackets */
    bool any = false;
    size_t j = i + 1;
    for (;;) {
        if ((punct(a, j, ".") || punct(a, j, "->")) && ident(a, j + 1)) { j += 2; continue; }
        if (!punct(a, j, "[")) break;
        resolution_begin(c);
        size_t past = column_access(c, i, j, &any);
        j = past ? past : index_group(c, i, j, &any);
        resolution_end(c);
    }
    return any;
}

static bool signature_of(const KAstNode *n, size_t i) {
    return n->body_first != SIZE_MAX ? i < n->body_first : n->kind == K_AST_FUNCTION;
}

/* `constexpr T NAME = value;` in a block: the name is the identifier just before
   the `=` (spec §4.2), and the value is what keel reads if it needs the number */
static void block_constexpr(Ctx *c, size_t i) {
    const KAst *a = c->ast;
    for (size_t j = i + 1; j < a->tokens.len && !punct(a, j, ";"); j++) {
        if (!punct(a, j, "=") || !ident(a, j - 1)) continue;
        size_t end = j + 1;
        while (end < a->tokens.len && !punct(a, end, ";")) end++;
        add_local(c, (Local){ .name = tok(a, j - 1), .decl_at = j - 1, .is_constexpr = true,
                              .value = range_text(a, (Range){ j + 1, end }) });
        return;
    }
}

static bool assignment_op(const KAst *a, size_t i) {
    static const char *const ops[] = { "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "++", "--" };
    for (size_t k = 0; k < sizeof ops / sizeof *ops; k++)
        if (punct(a, i, ops[k])) return true;
    return false;
}

/* an operand ends before token `i`: a name, a number, or a closing bracket */
static bool after_operand(const KAst *a, size_t i) {
    return i > 0 && (ident(a, i - 1) || k_token_is_number(tok(a, i - 1)) || punct(a, i - 1, ")") || punct(a, i - 1, "]"));
}

/* the uses a constant and a `ref` do not take (spec §4.2, items 13 and 15) */
static void check_uses(Ctx *c, size_t i) {
    const KAst *a = c->ast;
    KToken name = tok(a, i);
    const Local *l = find_local(c, name);
    if (l && l->decl_at == i) return;
    bool constant = l ? l->is_constexpr : false;
    if (!l) {
        const KSymbol *sym = k_symbol_resolve(a->symbols, name);
        constant = sym && sym->kind == K_SYM_CONSTANT && sym->origin;
    }
    bool address = i > 0 && punct(a, i - 1, "&") && !after_operand(a, i - 1);
    if (constant && (address || assignment_op(a, i + 1) || (i > 0 && (punct(a, i - 1, "++") || punct(a, i - 1, "--")))))
        diag3(c, K_DIAG_CONSTEXPR_AS_LVALUE, name, name, (keel_slice_char){0}, (keel_slice_char){0});
    if (l && l->is_ref) {
        bool before = i > 0 && (punct(a, i - 1, "++") || punct(a, i - 1, "--") ||
                                ((punct(a, i - 1, "+") || punct(a, i - 1, "-")) && after_operand(a, i - 1)));
        bool after = punct(a, i + 1, "+") || punct(a, i + 1, "-") || punct(a, i + 1, "+=") || punct(a, i + 1, "-=") ||
                     punct(a, i + 1, "++") || punct(a, i + 1, "--") || punct(a, i + 1, "[");
        if (before || after) diag3(c, K_DIAG_REF_ARITHMETIC, name, name, (keel_slice_char){0}, (keel_slice_char){0});
    }
}

/* ---- defer, foreach, walk, match (stages 4c, 4d, 4f) ----------------- */

static void put_range(Text *t, const KAst *a, size_t first, size_t end) {
    for (size_t j = first; j < end; j++) {
        if (j > first) {
            bool tight = punct(a, j, ".") || punct(a, j - 1, ".") || punct(a, j, "..") || punct(a, j - 1, "..") ||
                         punct(a, j, "->") || punct(a, j - 1, "->") || punct(a, j, ",") || punct(a, j - 1, "(") ||
                         punct(a, j, ")") || punct(a, j, "[") || punct(a, j - 1, "[") || punct(a, j, "]") ||
                         punct(a, j - 1, "*") || punct(a, j - 1, "&");
            if (!tight) put_str(t, " ");
            else if (punct(a, j - 1, ",")) put_str(t, " ");
        }
        put(t, tok(a, j).ptr, tok(a, j).len);
    }
}

/* the '{' that opens the block holding token `i`, or SIZE_MAX */
static size_t enclosing_brace(const KAst *a, size_t from, size_t i) {
    size_t depth = 0;
    for (size_t j = i; j-- > from; ) {
        if (punct(a, j, "}")) depth++;
        else if (punct(a, j, "{")) { if (!depth) return j; depth--; }
    }
    return SIZE_MAX;
}

/* the control word whose statement token `j` is the body of — `if`, `while`,
   `for`, `switch` before a ')', or `else` and `do' themselves — or NULL */
static const char *control_before(const KAst *a, size_t from, size_t j) {
    if (j <= from) return NULL;
    if (word(a, j - 1, "else")) return "else";
    if (word(a, j - 1, "do")) return "do";
    if (!punct(a, j - 1, ")")) return NULL;
    size_t depth = 0;
    for (size_t k = j - 1; k-- > from; ) {
        if (punct(a, k, ")")) depth++;
        else if (punct(a, k, "(")) {
            if (depth) { depth--; continue; }
            static const char *const words[] = { "if", "while", "for", "switch" };
            for (size_t w = 0; w < 4; w++) if (k > from && word(a, k - 1, words[w])) return words[w];
            return NULL;
        }
    }
    return NULL;
}

/* defer [capture] body (spec §4.6, cgen-tool §5.2: the capture as written, or
   `later` when none is written) */
static void defer_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    const char *ctl = control_before(a, n->first, at);
    if (ctl) diag3(c, K_DIAG_DEFER_WITHOUT_BRACES, tok(a, at), k_diag_text(ctl), none, none);
    size_t brace = enclosing_brace(a, n->first, at);
    const char *block = brace != SIZE_MAX ? control_before(a, n->first, brace) : NULL;
    if (block && (!strcmp(block, "if") || !strcmp(block, "else") || !strcmp(block, "switch")))
        diag3(c, K_DIAG_DEFER_IN_CONTROL_BLOCK, tok(a, at), k_diag_text(block), none, none);
    Text d = { .ok = true };
    if (punct(a, at + 1, "[")) {
        size_t close = close_bracket(a, at + 1, n->end);
        if (close == SIZE_MAX) return;
        if (word(a, at + 2, "later") && close > at + 3)
            diag3(c, K_DIAG_LATER_WITH_CAPTURE, tok(a, at + 3), range_text(a, (Range){ at + 3, close }), none, none);
        put_range(&d, a, at + 2, close);
    } else {
        put_str(&d, "later");
    }
    emit(c, K_ISLAND_DEFER, at, &d);
}

/* the protocols of the base live in the prelude (spec §5.1, rule 9) */
static const KAst *protocols_ast(const Ctx *c) {
    const KModule *m = module_by_name(c->ast->symbols, "keel");
    return m && m->ast ? m->ast : NULL;
}

/* Whether the type of `r`, which has kind KT_MODIFIER or KT_NAMED, meets the
   base protocol `proto`; `missing` names the verbs it lacks. A type the pass
   cannot see is taken as meeting it: C decides. */
static bool meets(Ctx *c, Range r, const char *proto, Text *missing) {
    int t = type_of(c, r);
    if (t < 0) return true;
    const KType *k = kt(c, t);
    if (k->kind != KT_MODIFIER && k->kind != KT_NAMED && k->kind != KT_ARRAY) return true;
    const KAst *pa = protocols_ast(c);
    KProtocolDecl pd;
    if (!pa || !protocol_decl(c, pa, k_diag_text(proto), &pd, 0)) return true;
    const KAst *own;
    if (k->kind == KT_ARRAY) {
        /* an `array` meets a protocol by the verbs of keel.array (spec §5.1) */
        const KModule *am = module_by_name(c->ast->symbols, "keel.array");
        own = am && am->ast ? am->ast : NULL;
        if (!own) { put_str(missing, "its verbs"); return false; }
        return satisfies(c, own, (keel_slice_char){0}, &pd, missing, tok(c->ast, r.first));
    }
    own = k->module && k->module->ast ? k->module->ast : c->ast;
    return satisfies(c, own, k->name, &pd, missing, tok(c->ast, r.first));
}

/* the statement after token `at`: a block to its '}', or a statement to its ';' */
static size_t statement_end(const KAst *a, size_t at, size_t end) {
    size_t depth = 0;
    for (size_t j = at; j < end; j++) {
        if (punct(a, j, "{") || punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, "}") || punct(a, j, ")") || punct(a, j, "]")) {
            if (depth && --depth == 0 && punct(a, j, "}") && punct(a, at, "{")) return j;
        }
        else if (!depth && punct(a, j, ";")) return j;
    }
    return end;
}

/* `push`, `pop` or `clear` over the container `box` in [first, end): the body
   of the construction that goes over it (spec §4.7, §4.8) */
static void mutations(Ctx *c, size_t first, size_t end, Range box, const char *what) {
    KAst *a = c->ast;
    keel_slice_char text = range_text(a, box);
    static const char *const verbs[] = { "push", "pop", "clear" };
    for (size_t j = first; j < end; j++) {
        if (!punct(a, j + 1, "(")) continue;
        bool verb = false;
        for (size_t v = 0; v < 3; v++) if (word(a, j, verbs[v])) verb = true;
        if (!verb) continue;
        size_t arg = j + 2, stop = arg;
        for (size_t depth = 0; stop < end; stop++) {
            if (punct(a, stop, "(") || punct(a, stop, "[")) depth++;
            else if ((punct(a, stop, ")") || punct(a, stop, "]")) && depth-- == 0) break;
            else if (!depth && punct(a, stop, ",")) break;
        }
        while (arg < stop && punct(a, arg, "&")) arg++;
        keel_slice_char got = range_text(a, (Range){ arg, stop });
        if (stop > arg && got.len == text.len && !memcmp(got.ptr, text.ptr, text.len))
            diag3(c, K_DIAG_MUTATION_DURING_TRAVERSAL, tok(a, j), tok(a, j), text, k_diag_text(what));
    }
}

/* foreach (binder[, binder] : container) and walk (elem, cursor : container)
   (spec §4.7). The detail is the binders' names, the container as written,
   and its keel type when it is a declared symbol (cgen-tool §5.2). */
static void traversal(Ctx *c, const KAstNode *n, size_t at, bool cursor) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t open = at + 1, close = close_paren(a, open, n->end);
    if (close == SIZE_MAX) return;
    size_t colon = SIZE_MAX, depth = 0;
    size_t comma[2]; size_t commas = 0;
    for (size_t j = open + 1; j < close; j++) {
        if (punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]")) depth--;
        else if (!depth && punct(a, j, ":")) { colon = j; break; }
        else if (!depth && punct(a, j, ",") && commas < 2) comma[commas++] = j;
    }
    if (colon == SIZE_MAX) return;
    size_t binders = commas + 1;
    Range b1 = { open + 1, commas ? comma[0] : colon }, b2 = { commas ? comma[0] + 1 : colon, colon };
    Range box = { colon + 1, close };
    bool literal = false, pointer = false;
    for (size_t j = box.first; j < box.end; j++) if (punct(a, j, "..")) literal = true;
    for (size_t j = b1.first; j < b1.end; j++) if (punct(a, j, "*")) pointer = true;

    Text d = { .ok = true };
    if (b1.end > b1.first) put(&d, tok(a, b1.end - 1).ptr, tok(a, b1.end - 1).len);
    if (binders > 1 && b2.end > b2.first) { put_str(&d, ", "); put(&d, tok(a, b2.end - 1).ptr, tok(a, b2.end - 1).len); }
    put_str(&d, " : ");
    put_range(&d, a, box.first, box.end);
    const Local *l = box.end == box.first + 1 && ident(a, box.first) ? find_local(c, tok(a, box.first)) : NULL;
    if (l && l->spec.text.len) { put_str(&d, " ("); put(&d, l->spec.text.ptr, l->spec.text.len); put_str(&d, ")"); }
    emit(c, cursor ? K_ISLAND_WALK : K_ISLAND_FOREACH, at, &d);
    if (!literal) mutations(c, close + 1, statement_end(a, close + 1, n->end), box, cursor ? "walk" : "foreach");

    Text missing = { .ok = true };
    if (cursor) {
        if (binders < 2) { diag3(c, K_DIAG_WALK_WITHOUT_CURSOR, tok(a, at), range_text(a, b1), none, none); return; }
        if (!literal && !meets(c, box, "Traversable", &missing))
            diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, box.first), range_text(a, box), k_diag_text("Traversable"),
                  keep(c, &missing));
        return;
    }
    if (binders == 2 && literal)
        diag3(c, K_DIAG_FOREACH_TWO_BINDERS_ON_LITERAL, tok(a, box.first), range_text(a, box), none, none);
    if (binders == 1 && literal && pointer)
        diag3(c, K_DIAG_POINTER_BINDER_ON_RANGE, tok(a, b1.first), range_text(a, b1), none, none);
    if (binders == 2 && !(b2.end == b2.first + 2 && word(a, b2.first, "size_t")))
        diag3(c, K_DIAG_INDEX_NOT_SIZE_T, tok(a, b2.first), range_text(a, b2), none, none);
    if (literal) return;
    int t = type_of(c, box);
    if (t >= 0 && kt(c, t)->kind == KT_ARRAY) return;          /* the core's own traversal (spec §4.2) */
    const char *proto = binders == 1 ? "Countable" : pointer ? "IndexPtr" : "IndexGet";
    if (!meets(c, box, proto, &missing)) {
        Text other = { .ok = true };
        if (binders == 2 && !pointer && meets(c, box, "IndexPtr", &other)) return;
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, box.first), range_text(a, box), k_diag_text(proto),
              keep(c, &missing));
    }
}

/* The tags declaration `name` in `where` (spec §4.9). */
static bool tags_decl(const KAst *where, keel_slice_char name, KTagsDecl *out) {
    for (size_t i = 0; i < where->nodes.len; i++) {
        const KAstNode *n = node_at(where, i);
        if (n->kind != K_AST_TAGS || !k_symtab_same_name(tok(where, n->name_first), name)) continue;
        size_t kw = n->first;
        while (kw < n->name_first && !word(where, kw, "tags")) kw++;
        KToken t = tok(where, kw);
        KLexer lexer;
        TKPpKind pp;
        k_lexer_init(&lexer, (keel_slice_char){ (size_t)(where->source.ptr + where->source.len - t.ptr), (char *)t.ptr }, NULL);
        KToken first = k_lexer_next(&lexer, &pp), next;
        KSymbol scratch[2];
        KSymbolTable none;
        k_symtab_init(&none, scratch, 2);
        return k_scan_tags_decl(&lexer, first, &none, out, &next, &pp);
    }
    return false;
}

/* The set a `match` operand's labels come from (spec §4.9): its declared type
   when that is a set, or else the set its module's `tag` returns — for an
   instance, the argument written for the module's `tags` parameter. */
static int match_set(Ctx *c, Range op, const KAst **where, keel_slice_char *set) {
    int t = type_of(c, op);
    if (t < 0) return -1;
    const KType *k = kt(c, t);
    const KAst *own = k->module && k->module->ast ? k->module->ast : c->ast;
    if (k->kind == KT_NAMED) {
        const KSymbol *s = k_symbol_resolve(c->ast->symbols, k->name);
        if (s && s->kind == K_SYM_TAGS) { *where = own; *set = k->name; return 1; }
    }
    if (k->kind != KT_MODIFIER && k->kind != KT_NAMED) return -1;
    Sig sig = find_verb(own, k_diag_text("tag"), 1);
    if (!sig.found || (!sig.p[0].is_array && !k_symtab_same_name(sig.p[0].base, k->name))) return 0;
    size_t s = sig.fn->first;
    while (s < sig.fn->name_first && (word(own, s, "pub") || word(own, s, "priv") || word(own, s, "inline")))
        s++;
    if (s >= sig.fn->name_first || !ident(own, s)) return -1;
    keel_slice_char ret = tok(own, s);
    if (k->kind == KT_MODIFIER) {
        const KAstNode *m = module_node(own);
        int idx = 0;
        for (size_t j = m->dim_first; j < m->dim_end; j++) if (ident(own, j)) idx++;
        for (size_t j = m->tags_first; j < m->tags_end; j++) {
            if (!ident(own, j)) continue;
            if (k_symtab_same_name(tok(own, j), ret) && idx < k->argc) {
                const KType *arg = kt(c, k->arg[idx]);
                *where = arg->module && arg->module->ast ? arg->module->ast : c->ast;
                *set = arg->name;
                return 1;
            }
            idx++;
        }
    }
    for (size_t i = 0; i < own->nodes.len; i++)
        if (node_at(own, i)->kind == K_AST_TAGS && k_symtab_same_name(tok(own, node_at(own, i)->name_first), ret)) {
            *where = own; *set = ret; return 1;
        }
    return 2;                                       /* `tag` returns no set */
}

/* match (operand) { LABEL: … } (spec §4.9). */
static void match_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t open = at + 1, close = close_paren(a, open, n->end);
    if (close == SIZE_MAX || !punct(a, close + 1, "{")) return;
    Range op = { open + 1, close };
    size_t body = close + 1, depth = 0, end = SIZE_MAX;
    for (size_t j = body; j < n->end; j++) {
        if (punct(a, j, "{")) depth++;
        else if (punct(a, j, "}") && --depth == 0) { end = j; break; }
    }
    if (end == SIZE_MAX) return;
    size_t labels[K_TAGS_MAX_ITEMS * 2], count = 0;
    depth = 0;
    for (size_t j = body + 1; j < end; j++) {
        if (punct(a, j, "{") || punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, "}") || punct(a, j, ")") || punct(a, j, "]")) depth--;
        else if (!depth && ident(a, j) && punct(a, j + 1, ":") &&
                 (punct(a, j - 1, "{") || punct(a, j - 1, ";") || punct(a, j - 1, "}") || punct(a, j - 1, ":")) &&
                 count < sizeof labels / sizeof *labels)
            labels[count++] = j;
    }
    Text d = { .ok = true };
    put_range(&d, a, op.first, op.end);
    const Local *l = op.end == op.first + 1 && ident(a, op.first) ? find_local(c, tok(a, op.first)) : NULL;
    if (l && l->spec.text.len) { put_str(&d, " ("); put(&d, l->spec.text.ptr, l->spec.text.len); put_str(&d, ")"); }
    put_str(&d, ":");
    for (size_t k = 0; k < count; k++) { put_str(&d, " "); put(&d, tok(a, labels[k]).ptr, tok(a, labels[k]).len); }
    emit(c, K_ISLAND_MATCH, at, &d);

    const KAst *where = NULL;
    keel_slice_char set = {0};
    int found = match_set(c, op, &where, &set);
    if (found == 0) {
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, op.first), range_text(a, op), k_diag_text("Taggable"), k_diag_text("tag"));
        return;
    }
    if (found == 2) { diag3(c, K_DIAG_MATCH_WITHOUT_TAGS, tok(a, op.first), range_text(a, op), none, none); return; }
    KTagsDecl tags;
    if (found != 1 || !tags_decl(where, set, &tags)) return;
    for (size_t k = 0; k < count; k++) {
        bool in = false;
        for (size_t m = 0; m < tags.item_count; m++) if (k_symtab_same_name(tok(a, labels[k]), tags.items[m])) in = true;
        if (!in) diag3(c, tag_of_some_set(a, tok(a, labels[k])) ? K_DIAG_TAG_FROM_OTHER_SET : K_DIAG_TAG_NOT_IN_SET,
                       tok(a, labels[k]), tok(a, labels[k]), set, none);
        for (size_t m = 0; m < k; m++)
            if (k_symtab_same_name(tok(a, labels[k]), tok(a, labels[m])))
                diag3(c, K_DIAG_DUPLICATE_TAG, tok(a, labels[k]), tok(a, labels[k]), set, none);
    }
    for (size_t m = 0; m < tags.item_count; m++) {
        bool labelled = false;
        for (size_t k = 0; k < count; k++) if (k_symtab_same_name(tok(a, labels[k]), tags.items[m])) labelled = true;
        if (!labelled) diag3(c, K_DIAG_TAG_WITHOUT_LABEL, tok(a, at), tags.items[m], set, none);
    }
}

/* ---- parallel, win, fail (stage 4e) ----------------------------------- */

static bool inside_parallel(const Ctx *c, size_t i) { return c->par_close && i > c->par_open && i < c->par_close; }

/* the ranges a `;` at depth 0 splits [first, end) into, up to `cap` */
static size_t split_semis(const KAst *a, size_t first, size_t end, Range *out, size_t cap) {
    size_t n = 0, depth = 0, st = first;
    for (size_t j = first; j <= end; j++) {
        if (j < end && (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{"))) depth++;
        else if (j < end && (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}"))) depth--;
        else if (j == end || (!depth && punct(a, j, ";"))) {
            if (n < cap) out[n] = (Range){ st, j };
            n++;
            st = j + 1;
        }
    }
    return n;
}

static size_t colon_in(const KAst *a, Range r) {
    for (size_t j = r.first, depth = 0; j < r.end; j++) {
        if (punct(a, j, "(") || punct(a, j, "[")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]")) depth--;
        else if (!depth && punct(a, j, ":")) return j;
    }
    return SIZE_MAX;
}

/* parallel NAME POLICY (w : 0..k; part : x [; (captures)]) { body } (spec §4.8).
   The detail is the name, the policy, the binders and the container as
   written, the container's keel type, the captures, and the `partition` the
   distribution calls (cgen-tool §5.2). The body's tokens are walked as usual:
   this only records where it is, for `win`, `fail`, `return` and the captures. */
static void parallel_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (!ident(a, at + 1) || punct(a, at + 2, "(") || word(a, at + 1, "ALL") || word(a, at + 1, "ANY")) {
        diag3(c, K_DIAG_UNNAMED_PARALLEL, tok(a, at), none, none, none);
        return;
    }
    KToken name = tok(a, at + 1);
    size_t open = at + 2;
    while (open < n->end && open < at + 6 && !punct(a, open, "(")) open++;
    if (!punct(a, open, "(")) return;
    Range policy = { at + 2, open };
    size_t close = close_paren(a, open, n->end);
    if (close == SIZE_MAX || !punct(a, close + 1, "{")) return;
    size_t body = close + 1, end = SIZE_MAX;
    for (size_t j = body, depth = 0; j < n->end; j++) {
        if (punct(a, j, "{")) depth++;
        else if (punct(a, j, "}") && --depth == 0) { end = j; break; }
    }
    if (end == SIZE_MAX) return;
    Range part[3];
    size_t parts = split_semis(a, open + 1, close, part, 3);
    if (parts < 2) return;
    size_t c1 = colon_in(a, part[0]), c2 = colon_in(a, part[1]);
    if (c1 == SIZE_MAX || c2 == SIZE_MAX || c1 == part[0].first || c2 == part[1].first) return;
    Range workers = { c1 + 1, part[0].end }, box = { c2 + 1, part[1].end };
    Range binder = { part[1].first, c2 };
    Range capture = { 0, 0 };
    if (parts > 2 && punct(a, part[2].first, "(") && part[2].end > part[2].first &&
        punct(a, part[2].end - 1, ")"))
        capture = (Range){ part[2].first + 1, part[2].end - 1 };

    Text d = { .ok = true };
    put(&d, name.ptr, name.len);
    put_str(&d, " ");
    put_range(&d, a, policy.first, policy.end);
    put_str(&d, " (");
    put(&d, tok(a, c1 - 1).ptr, tok(a, c1 - 1).len);
    put_str(&d, " : ");
    put_range(&d, a, workers.first, workers.end);
    put_str(&d, "; ");
    put(&d, tok(a, c2 - 1).ptr, tok(a, c2 - 1).len);
    put_str(&d, " : ");
    put_range(&d, a, box.first, box.end);
    const Local *l = box.end == box.first + 1 && ident(a, box.first) ? find_local(c, tok(a, box.first)) : NULL;
    if (l && l->spec.text.len) { put_str(&d, " ("); put(&d, l->spec.text.ptr, l->spec.text.len); put_str(&d, ")"); }
    if (capture.end > capture.first) { put_str(&d, "; ("); put_range(&d, a, capture.first, capture.end); put_str(&d, ")"); }
    put_str(&d, ")");

    resolution_begin(c);
    int t = type_of(c, box);
    const KType *k = t >= 0 ? kt(c, t) : NULL;
    if (k && k->kind == KT_MODIFIER && k->module && ast_of(c, k->module)) {
        const KAst *home = ast_of(c, k->module);
        Text sym = { .ok = true };
        bool adapt = false;
        if (container_verb(k, home, k_diag_text("partition"), 3, &sym, &adapt)) {
            put_str(&d, " \xe2\x86\x92 ");
            put(&d, sym.s, sym.n);
            if (adapt) put_str(&d, " &1");
            /* the binder's written type against the product of `partition` (§4.8,
               item 5): a `byref` product comes by pointer, so the stars go */
            Sig sig = find_verb(home, k_diag_text("partition"), 3);
            int r = sig.found ? ret_type(c, home, sig.fn, t) : -1;
            size_t last = binder.end - 1;
            while (last > binder.first && punct(a, last - 1, "*")) last--;
            int b = last > binder.first ? kt_from_text(c, a->symbols, range_text(a, (Range){ binder.first, last })) : -1;
            if (r >= 0 && b >= 0 && *kt(c, r)->symbol && *kt(c, b)->symbol &&
                strcmp(kt(c, r)->symbol, kt(c, b)->symbol) != 0)
            {
                Text prod = { .ok = true };
                put_str(&prod, kt(c, r)->symbol);
                diag3(c, K_DIAG_PARTITION_TYPE_MISMATCH, tok(a, binder.first),
                      range_text(a, (Range){ binder.first, last }), range_text(a, box), keep(c, &prod));
            }
        }
    }
    Text missing = { .ok = true };
    if (!meets(c, box, "Partitionable", &missing))
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, box.first), range_text(a, box), k_diag_text("Partitionable"),
              keep(c, &missing));
    resolution_end(c);
    emit(c, K_ISLAND_PARALLEL, at, &d);
    mutations(c, body, end, box, "parallel");

    /* the count of workers and the policy are constants (§4.8) */
    size_t dots = SIZE_MAX;
    for (size_t j = workers.first; j < workers.end; j++) if (punct(a, j, "..")) dots = j;
    long v;
    if (dots != SIZE_MAX && !decimal_of(c, (Range){ dots + 1, workers.end }, &v))
        diag3(c, K_DIAG_NONCONSTANT_PARALLEL, tok(a, dots + 1), range_text(a, (Range){ dots + 1, workers.end }), none, none);
    bool qualified = policy.end == policy.first + 3 && punct(a, policy.first + 1, ".");
    if (!word(a, policy.first, "ALL") && !word(a, policy.first, "ANY") && !qualified && !decimal_of(c, policy, &v))
        diag3(c, K_DIAG_NONCONSTANT_PARALLEL, tok(a, policy.first), range_text(a, policy), none, none);

    /* the name declares a `parallel.control` in the enclosing scope (§4.8), which
       the queries of keel.parallel read when that module is imported */
    const KModule *pm = module_by_name(a->symbols, "keel.parallel");
    if (pm && !c->quiet) {
        Text ty = { .ok = true };
        keel_slice_char alias = alias_of(a->symbols, pm);
        put(&ty, alias.ptr, alias.len);
        put_str(&ty, ".control");
        keel_slice_char text = keep(c, &ty);
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = enter_text(text, &lexer, &pp);
        if (k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE)
            add_local(c, (Local){ .name = name, .spec = spec, .decl_at = at + 1 });
    }
    for (size_t k2 = 0; k2 < c->par_count; k2++)
        if (k_symtab_same_name(c->par_names[k2], name))
            diag3(c, K_DIAG_DUPLICATE_PARALLEL_NAME, name, name, none, none);
    if (c->par_count < sizeof c->par_names / sizeof *c->par_names) c->par_names[c->par_count++] = name;
    if (inside_parallel(c, at)) {
        diag3(c, K_DIAG_NESTED_PARALLEL, tok(a, at), name, c->par_name, none);
        return;
    }
    c->par_open = body;
    c->par_close = end;
    c->par_name = name;
    c->cap_first = capture.first;
    c->cap_end = capture.end;
}

/* win; fail; return; and a write to a capture, in or out of a worker body */
static bool worker_flow(Ctx *c, size_t i) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    bool inside = inside_parallel(c, i);
    if ((word(a, i, "win") || word(a, i, "fail")) && punct(a, i + 1, ";")) {
        if (!inside) {
            diag3(c, K_DIAG_FLOW_VERB_OUTSIDE_PARALLEL, tok(a, i), tok(a, i), none, none);
            return true;
        }
        Text d = { .ok = true };
        put(&d, tok(a, i).ptr, tok(a, i).len);
        put_str(&d, " \xe2\x86\x92 ");
        put(&d, c->par_name.ptr, c->par_name.len);
        emit(c, K_ISLAND_WORKER_EXIT, i, &d);
        return true;
    }
    if (!inside) return false;
    bool write = assignment_op(a, i + 1) || (i > 0 && (punct(a, i - 1, "++") || punct(a, i - 1, "--")));
    bool through = i > 0 && punct(a, i - 1, "*") && !after_operand(a, i - 1);   /* `*out = v` writes the target */
    if (!write || through) return false;
    for (size_t j = c->cap_first; j < c->cap_end; j++) {
        if (!ident(a, j) || !k_symtab_same_name(tok(a, j), tok(a, i))) continue;
        const Local *l = find_local(c, tok(a, i));
        if (l && l->spec.kind == K_SPEC_MODIFIER && l->pointers == 0) return false;   /* a byref instance */
        diag3(c, K_DIAG_CAPTURED_WRITE, tok(a, i), tok(a, i), c->par_name, none);
        return false;
    }
    return false;
}

/* ---- extent (stage 4h) ------------------------------------------------ */

#define K_EXTENT_MAX 16

typedef struct {
    const KAst *home;
    size_t at;                              /* the word `extent` */
    keel_slice_char name;
    size_t groups;
    size_t count[8], cap[8];                /* token indexes of each group's names */
    size_t columns;
    size_t col[K_EXTENT_MAX];               /* the column's name */
    bool embedded[K_EXTENT_MAX];
    keel_slice_char dims[K_EXTENT_MAX];     /* an embedded column's dimensions, as written */
    size_t fields;
    keel_slice_char field[32];              /* every field's name, columns included */
} Extent;

/* `extent struct NAME [count, cap]... { fields }` in [n->first, n->end) of `home` (spec §4.11) */
static bool extent_scan(const KAst *home, const KAstNode *n, Extent *e) {
    memset(e, 0, sizeof *e);
    e->home = home;
    size_t j = n->first;
    while (j < n->end && (word(home, j, "pub") || word(home, j, "priv"))) j++;
    if (!word(home, j, "extent") || !word(home, j + 1, "struct") || !ident(home, j + 2)) return false;
    e->at = j;
    e->name = tok(home, j + 2);
    j += 3;
    while (punct(home, j, "[") && e->groups < 8) {
        size_t cl = close_bracket(home, j, n->end);
        if (cl == SIZE_MAX || cl != j + 4 || !punct(home, j + 2, ",")) return false;
        e->count[e->groups] = j + 1;
        e->cap[e->groups++] = j + 3;
        j = cl + 1;
    }
    if (!punct(home, j, "{")) return false;
    size_t close = j, depth = 0;
    for (; close < n->end; close++) {
        if (punct(home, close, "{")) depth++;
        else if (punct(home, close, "}") && --depth == 0) break;
    }
    for (size_t f = j + 1; f < close; ) {
        size_t end = f;
        depth = 0;
        while (end < close && !(depth == 0 && punct(home, end, ";"))) {
            if (punct(home, end, "(") || punct(home, end, "[") || punct(home, end, "{")) depth++;
            else if (punct(home, end, ")") || punct(home, end, "]") || punct(home, end, "}")) depth--;
            end++;
        }
        bool column = word(home, f, "array");
        /* each declarator: the last name before a ',' or a '[' at depth 0 */
        size_t name = SIZE_MAX, dims = SIZE_MAX;
        depth = 0;
        for (size_t k = f; k <= end; k++) {
            bool stop = k == end || (!depth && punct(home, k, ","));
            if (!stop) {
                if (punct(home, k, "[") && !depth && dims == SIZE_MAX) dims = k;
                if (punct(home, k, "(") || punct(home, k, "[") || punct(home, k, "{")) depth++;
                else if (punct(home, k, ")") || punct(home, k, "]") || punct(home, k, "}")) depth--;
                else if (!depth && dims == SIZE_MAX && ident(home, k)) name = k;
                continue;
            }
            if (name != SIZE_MAX) {
                if (e->fields < 32) e->field[e->fields++] = tok(home, name);
                if (column && e->columns < K_EXTENT_MAX) {
                    e->col[e->columns] = name;
                    e->embedded[e->columns] = dims != SIZE_MAX;
                    if (dims != SIZE_MAX)
                        e->dims[e->columns] = range_text(home, (Range){ dims, k });
                    e->columns++;
                }
            }
            name = dims = SIZE_MAX;
        }
        f = end + 1;
    }
    return true;
}

static bool extent_field(const Extent *e, keel_slice_char name) {
    for (size_t k = 0; k < e->fields; k++) if (k_symtab_same_name(e->field[k], name)) return true;
    return false;
}

/* the extent declared as `name` in `home`, if it is one */
static bool extent_named(const KAst *home, keel_slice_char name, Extent *e) {
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *n = node_at(home, i);
        if (n->kind == K_AST_TYPE && k_symtab_same_name(tok(home, n->name_first), name)) return extent_scan(home, n, e);
    }
    return false;
}

/* the declaration: its groups, its columns and their checks */
static void extent_island(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    Extent e;
    if (!extent_scan(a, n, &e)) return;
    bool any_embedded = false, any_pointer = false;
    for (size_t k = 0; k < e.columns; k++) {
        if (e.embedded[k]) any_embedded = true;
        else any_pointer = true;
    }
    Text d = { .ok = true };
    put(&d, e.name.ptr, e.name.len);
    for (size_t g = 0; g < e.groups; g++) {
        put_str(&d, " [");
        put(&d, tok(a, e.count[g]).ptr, tok(a, e.count[g]).len);
        put_str(&d, ", ");
        put(&d, tok(a, e.cap[g]).ptr, tok(a, e.cap[g]).len);
        put_str(&d, "]");
    }
    put_str(&d, any_embedded ? " embedded:" : " pointer:");
    for (size_t k = 0; k < e.columns; k++) { put_str(&d, " "); put(&d, tok(a, e.col[k]).ptr, tok(a, e.col[k]).len); }
    emit(c, K_ISLAND_EXTENT, e.at, &d);

    bool field_cap = false;
    for (size_t g = 0; g < e.groups; g++) {
        KToken count = tok(a, e.count[g]), cap = tok(a, e.cap[g]);
        if (!extent_field(&e, count)) diag3(c, K_DIAG_EXTENT_COUNT_NOT_FIELD, count, count, e.name, none);
        long v;
        if (extent_field(&e, cap)) field_cap = true;
        else if (!decimal_of_token(c, cap, &v)) diag3(c, K_DIAG_EXTENT_UNKNOWN_CAPACITY, cap, cap, e.name, none);
    }
    if (!e.columns) { diag3(c, K_DIAG_EXTENT_WITHOUT_COLUMN, tok(a, e.at), e.name, none, none); return; }
    if (any_embedded && any_pointer) diag3(c, K_DIAG_EXTENT_MIXED_COLUMNS, tok(a, e.at), e.name, none, none);
    Text caps = { .ok = true };
    put_str(&caps, "[");
    for (size_t g = 0; g < e.groups; g++) {
        if (g) put_str(&caps, ", ");
        put(&caps, tok(a, e.cap[g]).ptr, tok(a, e.cap[g]).len);
    }
    put_str(&caps, "]");
    for (size_t k = 0; k < e.columns; k++) {
        if (!e.embedded[k]) continue;
        KToken col = tok(a, e.col[k]);
        if (field_cap) {
            for (size_t g = 0; g < e.groups; g++)
                if (extent_field(&e, tok(a, e.cap[g]))) {
                    diag3(c, K_DIAG_EXTENT_EMBEDDED_FIELD_CAPACITY, col, col, tok(a, e.cap[g]), none);
                    break;
                }
            continue;
        }
        /* the dimensions, one per group and named as its capacity */
        KLexer lexer;
        TKPpKind pp;
        k_lexer_init(&lexer, e.dims[k], NULL);
        size_t g = 0, depth = 0, tokens = 0;
        bool same = true;
        KToken only = {0};
        for (KToken t = k_lexer_next(&lexer, &pp); t.len; t = k_lexer_next(&lexer, &pp)) {
            bool open = k_token_is_punct(t, "["), close = k_token_is_punct(t, "]");
            if (open && depth++ == 0) { tokens = 0; continue; }
            if ((close && --depth == 0) || (depth == 1 && k_token_is_punct(t, ","))) {
                if (g >= e.groups || tokens != 1 || !k_symtab_same_name(only, tok(a, e.cap[g]))) same = false;
                g++;
                tokens = 0;
                continue;
            }
            if (depth >= 1) { tokens++; only = t; }
        }
        if (!same || g != e.groups)
            diag3(c, K_DIAG_EXTENT_DIMENSION_MISMATCH, col, col, e.dims[k], keep(c, &caps));
    }
}

/* `P.col[i, ...]` or `P->col[...]`, with `col` a column of an extent: the
   path [i, open) ends in `. col` or `-> col` (spec §4.11). Returns the token
   after the brackets, or 0 when the path is not a column. */
static size_t column_access(Ctx *c, size_t i, size_t open, bool *any) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (open < i + 3 || !ident(a, open - 1) || !(punct(a, open - 2, ".") || punct(a, open - 2, "->"))) return 0;
    int t = type_of(c, (Range){ i, open - 2 });
    if (t < 0) return 0;
    const KType *k = kt(c, t);
    if (k->kind != KT_NAMED || !*k->symbol) return 0;
    const KAst *home = ast_of(c, k->module);
    Extent e;
    if (!home || !extent_named(home, k->name, &e)) return 0;
    size_t which = SIZE_MAX;
    for (size_t m = 0; m < e.columns; m++) if (k_symtab_same_name(tok(home, e.col[m]), tok(a, open - 1))) which = m;
    if (which == SIZE_MAX) return 0;
    size_t close = close_bracket(a, open, a->tokens.len);
    if (close == SIZE_MAX) return 0;
    Range idx[8];
    size_t indices = 0, st = open + 1, depth = 0;
    for (size_t m = open + 1; m <= close; m++) {
        if (m < close && (punct(a, m, "(") || punct(a, m, "[") || punct(a, m, "{"))) depth++;
        else if (m < close && (punct(a, m, ")") || punct(a, m, "]") || punct(a, m, "}"))) depth--;
        else if (m == close || (!depth && punct(a, m, ","))) {
            if (indices < 8) idx[indices] = (Range){ st, m };
            indices++;
            st = m + 1;
        }
    }
    keel_slice_char path = { (size_t)(tok(a, open - 1).ptr + tok(a, open - 1).len - tok(a, i).ptr), tok(a, i).ptr };
    Text d = { .ok = true };
    put(&d, path.ptr, path.len);
    put_str(&d, " rank ");
    put_uint(&d, indices);
    put_str(&d, " \xe2\x86\x92 ");
    put_str(&d, k->symbol);
    put_str(&d, "_");
    put(&d, tok(a, open - 1).ptr, tok(a, open - 1).len);
    put_str(&d, "_ptr");
    if (punct(a, open - 2, ".")) put_str(&d, " &1");
    *any = emit(c, K_ISLAND_COLUMN, i, &d) || *any;
    if (indices != e.groups) {
        Text r = { .ok = true }, w = { .ok = true };
        put_uint(&r, e.groups);
        put_uint(&w, indices);
        diag3(c, K_DIAG_EXTENT_INDEX_ARITY, tok(a, i), path, keep(c, &r), keep(c, &w));
        return close + 1;
    }
    for (size_t g = 0; g < indices && g < 8; g++) {
        long v, cap;
        if (decimal_of(c, idx[g], &v) && decimal_of_token(c, tok(home, e.cap[g]), &cap) && v >= cap)
            diag3(c, K_DIAG_EXTENT_INDEX_ABOVE_CAPACITY, tok(a, idx[g].first), range_text(a, idx[g]),
                  tok(home, e.cap[g]), none);
    }
    return close + 1;
}

/* `f(x)->col[i]` or `f(x).col[i]`: a column reached through a call, where `col`
   is a column of an extent of the file (spec §4.11) */
static void column_through_call(Ctx *c, const KAstNode *n, size_t i) {
    KAst *a = c->ast;
    if (!punct(a, i, ")") || !(punct(a, i + 1, ".") || punct(a, i + 1, "->")) || !ident(a, i + 2) ||
        !punct(a, i + 3, "["))
        return;
    for (size_t m = 0; m < a->nodes.len; m++) {
        const KAstNode *t = node_at(a, m);
        Extent e;
        if (t->kind != K_AST_TYPE || !extent_scan(a, t, &e)) continue;
        for (size_t k = 0; k < e.columns; k++)
            if (k_symtab_same_name(tok(a, e.col[k]), tok(a, i + 2))) {
                size_t start = i, depth = 0;
                while (start > n->first) {
                    if (punct(a, start, ")")) depth++;
                    else if (punct(a, start, "(") && --depth == 0) break;
                    start--;
                }
                if (start > n->first && ident(a, start - 1)) start--;
                keel_slice_char path = { (size_t)(tok(a, i + 2).ptr + tok(a, i + 2).len - tok(a, start).ptr),
                                         tok(a, start).ptr };
                diag3(c, K_DIAG_EXTENT_PATH_WITH_CALL, tok(a, start), path, (keel_slice_char){0}, (keel_slice_char){0});
                return;
            }
    }
}

/* ---- the else of a result (stage 4g) ----------------------------------- */

/* `T x = e else …;` and `x = e else …;` (spec §4.10). The `else` of an `if`
   follows a statement, so a ';' or a '}' stands before it; the `else` of a
   result follows an expression. The detail is the target, its keel type, the
   form, and the verbs the test calls: `failed`, and `win` for a default
   (cgen-tool §5.2). */
static void else_island(Ctx *c, const KAstNode *n, size_t at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t s = at, depth = 0;
    while (s > n->first) {
        size_t j = s - 1;
        if (punct(a, j, ")") || punct(a, j, "]")) depth++;
        else if (punct(a, j, "(") || punct(a, j, "[")) { if (!depth) break; depth--; }
        else if (!depth && (punct(a, j, ";") || punct(a, j, "{") || punct(a, j, "}"))) break;
        s = j;
    }
    size_t eq = SIZE_MAX, commas = 0;
    depth = 0;
    for (size_t j = s; j < at; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) depth--;
        else if (!depth && punct(a, j, "=") && eq == SIZE_MAX) eq = j;
        else if (!depth && punct(a, j, ",")) commas++;
    }
    KLexer lexer;
    TKPpKind pp;
    KSpecifier spec;
    KToken next = {0};
    size_t head = s;
    while (head < at && (word(a, head, "const") || word(a, head, "volatile") || word(a, head, "static") ||
                         word(a, head, "register")))
        head++;
    KToken first = enter(a, head, &lexer, &pp);
    bool keel_decl = k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE;
    if (keel_decl && !(spec.kind == K_SPEC_NAMED_TYPE && k_primitive_name(spec.type_name))) {
        if (commas) { diag3(c, K_DIAG_ELSE_MULTIPLE_DECLARATORS, tok(a, at), none, none, none); return; }
        if (eq == SIZE_MAX) { diag3(c, K_DIAG_ELSE_WITHOUT_INITIALIZER, tok(a, at), none, none, none); return; }
    } else if (eq == SIZE_MAX) {
        diag3(c, K_DIAG_ELSE_WITHOUT_INITIALIZER, tok(a, at), none, none, none);
        return;
    } else if (eq != s + 1 || !ident(a, s)) {
        /* a C declaration — two names before the '=' — takes no `else`: its type
           is not keel's to test; anything else is a target that is not a name */
        bool c_decl = eq >= s + 2 && (ident(a, s) || k_token_is_c_word(tok(a, s))) && ident(a, eq - 1) &&
                      (ident(a, eq - 2) || k_token_is_c_word(tok(a, eq - 2)) ||
                                                            punct(a, eq - 2, "*"));
        for (size_t j = s; c_decl && j < eq; j++)
            if (punct(a, j, ".") || punct(a, j, "->") || punct(a, j, "[") || punct(a, j, "(")) c_decl = false;
        if (c_decl) diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, eq - 1), range_text(a, (Range){ head, eq - 1 }),
                          k_diag_text("Failable"), k_diag_text("failed"));
        else diag3(c, K_DIAG_ELSE_ON_COMPLEX_TARGET, tok(a, s), range_text(a, (Range){ s, eq }), none, none);
        return;
    }
    if (!ident(a, eq - 1)) return;
    Range target = { eq - 1, eq };
    const Local *l = find_local(c, tok(a, eq - 1));
    if (!keel_decl && (!l || !l->spec.text.len)) {
        diag3(c, K_DIAG_ELSE_ON_COMPLEX_TARGET, tok(a, s), range_text(a, target), none, none);
        return;
    }
    bool exit = punct(a, at + 1, "{") || word(a, at + 1, "return") || word(a, at + 1, "break") ||
                word(a, at + 1, "continue") || word(a, at + 1, "goto");

    Text d = { .ok = true };
    put(&d, tok(a, eq - 1).ptr, tok(a, eq - 1).len);
    if (l && l->spec.text.len) { put_str(&d, " ("); put(&d, l->spec.text.ptr, l->spec.text.len); put_str(&d, ")"); }
    put_str(&d, exit ? " exit" : " default");
    Text missing = { .ok = true };
    resolution_begin(c);
    int t = type_of(c, target);
    const KType *k = t >= 0 ? kt(c, t) : NULL;
    if (!meets(c, target, "Failable", &missing))
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, eq - 1), range_text(a, target), k_diag_text("Failable"),
              keep(c, &missing));
    else if (!exit && (missing = (Text){ .ok = true }, !meets(c, target, "Winnable", &missing)))
        diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, eq - 1), range_text(a, target), k_diag_text("Winnable"),
              keep(c, &missing));
    else if (k && (k->kind == KT_MODIFIER || k->kind == KT_NAMED) && k->module && ast_of(c, k->module)) {
        const KAst *home = ast_of(c, k->module);
        Text sym = { .ok = true };
        bool adapt = false, win_adapt = false;
        if (container_verb(k, home, k_diag_text("failed"), 1, &sym, &adapt) &&
            (exit || (put_str(&sym, " "), container_verb(k, home, k_diag_text("win"), 2, &sym, &win_adapt)))) {
            put_str(&d, " \xe2\x86\x92 ");
            put(&d, sym.s, sym.n);
            if (win_adapt) put_str(&d, " &1");
        }
    }
    resolution_end(c);
    emit(c, K_ISLAND_ELSE, at, &d);
}

static void walk(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    memcpy(c->locals, c->globals, c->global_count * sizeof *c->globals);
    c->local_count = c->global_count;
    c->depth = 0;
    c->has_target = false;
    c->par_open = c->par_close = 0;
    c->par_count = 0;
    c->cap_first = c->cap_end = 0;
    for (size_t i = n->first; i < n->end && !c->failed; i++) {
        if (!live(a, i)) continue;
        if (punct(a, i, "{")) { c->depth++; continue; }
        if (punct(a, i, "}")) {
            while (c->local_count && c->locals[c->local_count - 1].depth >= c->depth) c->local_count--;
            c->depth--;
            continue;
        }
        if (punct(a, i, ";")) { c->has_target = false; continue; }
        if (!signature_of(n, i) && punct(a, i, ")")) { column_through_call(c, n, i); continue; }
        if (word(a, i, "constexpr") && !signature_of(n, i)) {
            check_constexpr(c, i, n->end);
            block_constexpr(c, i);
            continue;
        }
        if (!signature_of(n, i) && word(a, i, "else") && i > n->first && !punct(a, i - 1, ";") &&
            !punct(a, i - 1, "}")) {
            else_island(c, n, i);
            continue;
        }
        if (word(a, i, "return") && inside_parallel(c, i)) {      /* a C word, not an identifier */
            diag3(c, K_DIAG_RETURN_IN_PARALLEL, tok(a, i), c->par_name, (keel_slice_char){0}, (keel_slice_char){0});
            continue;
        }
        if (i == n->name_first || !ident(a, i)) continue;
        if (i > n->first && (punct(a, i - 1, ".") || punct(a, i - 1, "->"))) continue;

        KToken t = tok(a, i);
        bool signature = signature_of(n, i);
        if (!signature && word(a, i, "defer")) { defer_island(c, n, i); continue; }
        if (!signature && word(a, i, "extern_c") && (punct(a, i + 1, "{") || punct(a, i + 1, "["))) {
            /* recognized only to be refused (spec §2.2, §4.1): its body is C */
            diag3(c, K_DIAG_NESTED_EXTERN_C, tok(a, i), (keel_slice_char){0}, (keel_slice_char){0}, (keel_slice_char){0});
            size_t b = i + 1;
            while (b < n->end && !punct(a, b, "{")) b++;
            for (size_t depth = 0; b < n->end; b++) {
                if (punct(a, b, "{")) depth++;
                else if (punct(a, b, "}") && --depth == 0) break;
            }
            i = b;
            continue;
        }
        if (!signature && word(a, i, "instance") && ident(a, i + 1) && !punct(a, i + 1, "=")) {
            size_t e = i + 1;
            while (e < n->end && !punct(a, e, ";") && !punct(a, e, "=") && !punct(a, e, "{")) e++;
            if (punct(a, e, ";")) {
                diag3(c, K_DIAG_INSTANCE_OUTSIDE_FILE_SCOPE, tok(a, i), range_text(a, (Range){ i + 1, e }),
                      (keel_slice_char){0}, (keel_slice_char){0});
                i = e;
                continue;
            }
        }
        if (!signature && punct(a, i + 1, "(") && (word(a, i, "foreach") || word(a, i, "walk"))) {
            resolution_begin(c);
            traversal(c, n, i, word(a, i, "walk"));
            resolution_end(c);
            continue;
        }
        if (!signature && word(a, i, "match") && punct(a, i + 1, "(")) {
            resolution_begin(c);
            match_island(c, n, i);
            resolution_end(c);
            continue;
        }
        if (!signature && word(a, i, "parallel") && !punct(a, i + 1, ".")) { parallel_island(c, n, i); continue; }
        if (!signature && worker_flow(c, i)) continue;
        check_uses(c, i);

        /* the marker, unless a qualifier: `array.get(`, with `array` an alias */
        if (k_token_spelled(t, "array") && !punct(a, i + 1, ".")) {
            i = array_decl(c, i, signature);
            continue;
        }
        if (k_token_spelled(t, "ref") && ident(a, i + 1) &&
            (punct(a, i - 1, "*") || word(a, i - 1, "const") || word(a, i - 1, "volatile"))) {
            Text d = { .ok = true };
            put(&d, tok(a, i + 1).ptr, tok(a, i + 1).len);
            emit(c, K_ISLAND_REF, i, &d);
            if (!signature && !punct(a, i + 2, "="))
                diag3(c, K_DIAG_REF_WITHOUT_INITIALIZER, tok(a, i + 1), tok(a, i + 1), (keel_slice_char){0},
                      (keel_slice_char){0});
            add_local(c, (Local){ .name = tok(a, i + 1), .pointers = 1, .decl_at = i + 1, .is_ref = true });
            continue;
        }

        /* a keel type, qualified or not */
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = enter(a, i, &lexer, &pp);
        bool scanned = k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp);
        if (scanned && spec.kind != K_SPEC_NONE) {
            if (spec.kind == K_SPEC_MODIFIER && i > n->first && word(a, i - 1, "restrict"))
                diag3(c, K_DIAG_RESTRICT_ON_CONTAINER, tok(a, i - 1), spec.text, (keel_slice_char){0},
                      (keel_slice_char){0});
            i = declaration(c, i, &spec, &lexer, next, signature, true);
            continue;
        }
        if (!scanned && punct(a, i + 1, "(") == false) {
            /* a modifier that reads a C arithmetic keyword as its argument (spec §4.2, item 6) */
            const KSymbol *m = k_symbol_resolve(a->symbols, t);
            static const char *const arithmetic[] = { "int", "long", "short", "unsigned", "signed", "float", "double" };
            if (m && m->kind == K_SYM_MODIFIER)
                for (size_t k = 0; k < sizeof arithmetic / sizeof *arithmetic; k++)
                    if (word(a, i + 1, arithmetic[k])) {
                        diag3(c, K_DIAG_C_TYPE_AS_ARGUMENT, tok(a, i + 1), tok(a, i), tok(a, i + 1),
                              (keel_slice_char){0});
                        break;
                    }
        }

        bool stmt_start = i == n->body_first + 1 || punct(a, i - 1, ";") || punct(a, i - 1, "{") || punct(a, i - 1, "}");
        /* `IDENT IDENT` or `IDENT * IDENT` opening a statement, the second a
           known keel symbol: a possible redeclaration, refused (spec §2.5) */
        if (!signature && stmt_start) {
            size_t second = punct(a, i + 1, "*") ? i + 2 : i + 1;
            if (ident(a, second) && (punct(a, second + 1, ";") || punct(a, second + 1, "=") ||
                                     punct(a, second + 1, ",") || punct(a, second + 1, "["))) {
                KToken nm = tok(a, second);
                const Local *l = find_local(c, nm);
                const KSymbol *sym = l ? NULL : k_symbol_resolve(a->symbols, nm);
                bool known = (l && l->spec.kind != K_SPEC_NONE) ||
                             (sym && sym->origin && (sym->kind == K_SYM_VARIABLE || sym->kind == K_SYM_FUNCTION ||
                                                     sym->kind == K_SYM_CONSTANT));
                if (known) diag3(c, K_DIAG_SYMBOL_REDECLARATION, nm, nm, tok(a, i), nm);
            }
        }
        /* `a = b;` between two `byref` instances by value: one storage, two
           names (spec §4.3, rule 17) */
        if (!signature && stmt_start && punct(a, i + 1, "=") && ident(a, i + 2) && punct(a, i + 3, ";")) {
            const Local *l = find_local(c, tok(a, i)), *r = find_local(c, tok(a, i + 2));
            if (l && r && l->pointers == 0 && r->pointers == 0 && l->spec.kind != K_SPEC_NONE &&
                r->spec.kind != K_SPEC_NONE && spec_byref(a, &l->spec) && spec_byref(a, &r->spec))
                diag3(c, K_DIAG_BYREF_ASSIGNMENT, tok(a, i), tok(a, i), tok(a, i + 2), (keel_slice_char){0});
        }

        /* `alias.verb(`, `verb(x, ...)` over a container, and a function of the file */
        if (call_at(c, i)) continue;
        if (!find_local(c, t) && punct(a, i + 1, ".") && ident(a, i + 2) && !punct(a, i + 3, "(")) {
            const KModule *module = module_alias(a->symbols, t);
            if (module) { qualified_name(c, i, module); i += 2; continue; }
        }
        if (punct(a, i + 1, "(")) continue;             /* a C function: not ours */
        /* a label is not a use */
        if (punct(a, i + 1, ":") && (i == n->first || punct(a, i - 1, "{") || punct(a, i - 1, ";") ||
                                     punct(a, i - 1, "}") || punct(a, i - 1, ":")))
            continue;
        if ((punct(a, i + 1, "[") || punct(a, i + 1, ".") || punct(a, i + 1, "->")) && path_forms(c, i)) continue;
        name_island(c, i);
    }
}

/* ---- the closure of the instances (stage 4i) --------------------------- */

static bool qualified_const(const char *symbol) { return !strncmp(symbol, "const_", 6); }

/* the type written for parameter `p`, without its name, role and stars */
static Range param_type(const KAst *home, const Param *p) {
    Range r = { p->first, p->end ? p->end - 1 : p->first };
    if (p->role != K_ROLE_NONE) r.first++;
    while (r.end > r.first && punct(home, r.end - 1, "*")) r.end--;
    return r;
}

/* the type written for `name` where it is declared in `fn` — a parameter, or a
   declaration in the body before token `before` */
static bool declared_type(const KAst *home, const KAstNode *fn, KToken name, size_t before, keel_slice_char *out) {
    Param p[K_PARAMS_MAX];
    size_t n = params_of(home, fn, p, K_PARAMS_MAX);
    for (size_t k = 0; n != SIZE_MAX && k < n; k++)
        if (p[k].end > p[k].first && k_symtab_same_name(tok(home, p[k].end - 1), name)) {
            Range r = param_type(home, &p[k]);
            if (r.end <= r.first) return false;
            *out = range_text(home, r);
            return true;
        }
    for (size_t j = fn->body_first; j < before && j < fn->body_end; j++) {
        if (!k_symtab_same_name(tok(home, j), name) || !(punct(home, j + 1, "=") || punct(home, j + 1, ";"))) continue;
        size_t st = j;
        while (st > fn->body_first && !punct(home, st - 1, ";") && !punct(home, st - 1, "{") && !punct(home, st - 1, "}"))
            st--;
        size_t e = j;
        while (e > st && punct(home, e - 1, "*")) e--;
        if (e <= st) return false;
        *out = range_text(home, (Range){ st, e });
        return true;
    }
    return false;
}

/* the fields of the modifiers of `home` whose type is binder `b`: by value
   (`T value`) or as the pointee (`T *ptr`) */
static bool binder_field(const KAst *home, keel_slice_char b, keel_slice_char field, bool *pointer) {
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *m = node_at(home, i);
        if (m->kind != K_AST_MODIFIER || m->body_first == SIZE_MAX) continue;
        for (size_t j = m->body_first; j + 2 < m->body_end; j++) {
            if (!k_symtab_same_name(tok(home, j), b)) continue;
            if (j > m->body_first && !punct(home, j - 1, ";") && !punct(home, j - 1, "{")) continue;
            if (ident(home, j + 1) && k_symtab_same_name(tok(home, j + 1), field)) { *pointer = false; return true; }
            if (punct(home, j + 1, "*") && ident(home, j + 2) && k_symtab_same_name(tok(home, j + 2), field)) {
                *pointer = true;
                return true;
            }
        }
    }
    return false;
}

static bool verb_available(Ctx *c, const KAst *home, const KAstNode *fn, int t, const char **cause);

/* the arguments of the call whose '(' is `open`, up to `close` */
static size_t call_args(const KAst *a, size_t open, size_t close, Range *first) {
    size_t n = 0, depth = 0, st = open + 1;
    if (close == open + 1) return 0;
    for (size_t j = open + 1; j <= close; j++) {
        if (j < close && (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{"))) depth++;
        else if (j < close && (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}"))) depth--;
        else if (j == close || (!depth && punct(a, j, ","))) {
            if (n == 0) *first = (Range){ st, j };
            n++;
            st = j + 1;
        }
    }
    return n;
}

/* Spec §4.3, rules 11 to 13: a verb of an instance whose argument is `void`
   loses the reads and writes of the omitted `T value` and the parameters of
   type `T` by value; one whose argument is `const` loses the writes to the
   constant value; and a verb that calls a lost verb is lost with it. */
static bool compute_surface(Ctx *c, const KAst *home, const KAstNode *fn, int t, Text *why) {
    const KType *k = kt(c, t);
    keel_slice_char binders[8];
    size_t nb = binders_of(home, binders);
    bool is_void[8] = {0}, is_const[8] = {0};
    for (size_t b = 0; b < nb && b < (size_t)k->argc; b++) {
        const char *sym = kt_symbol(c, k->arg[b]);
        is_void[b] = !strcmp(sym, "void");
        is_const[b] = qualified_const(sym);
    }
    Param p[K_PARAMS_MAX];
    size_t np = params_of(home, fn, p, K_PARAMS_MAX);
    for (size_t q = 0; np != SIZE_MAX && q < np; q++) {
        Range r = param_type(home, &p[q]);
        if (r.end != r.first + 1 || p[q].pointer) continue;
        for (size_t b = 0; b < nb; b++)
            if (is_void[b] && k_symtab_same_name(tok(home, r.first), binders[b])) {
                put_str(why, "void-param ");
                put(why, tok(home, p[q].end - 1).ptr, tok(home, p[q].end - 1).len);
                return false;
            }
    }
    if (fn->body_first == SIZE_MAX) return true;
    for (size_t j = fn->body_first; j < fn->body_end; j++) {
        /* a field of the instance: `x->value`, `x.ptr[i] = v` */
        if (ident(home, j) && j > fn->body_first && (punct(home, j - 1, "->") || punct(home, j - 1, ".")) &&
            !punct(home, j + 1, "(")) {
            for (size_t b = 0; b < nb; b++) {
                bool pointer;
                if ((!is_void[b] && !is_const[b]) || !binder_field(home, binders[b], tok(home, j), &pointer)) continue;
                if (is_void[b] && !pointer) {
                    put_str(why, "void-field ");
                    put(why, tok(home, j).ptr, tok(home, j).len);
                    return false;
                }
                if (!is_const[b]) continue;
                size_t after = j + 1;
                if (pointer) {
                    if (!punct(home, after, "[")) continue;
                    after = close_bracket(home, after, fn->body_end);
                    if (after == SIZE_MAX) continue;
                    after++;
                }
                if (assignment_op(home, after) || (j >= 3 && (punct(home, j - 3, "++") || punct(home, j - 3, "--")))) {
                    put_str(why, "const-write ");
                    put(why, tok(home, j).ptr, tok(home, j).len);
                    return false;
                }
            }
            continue;
        }
        /* a known keel call: `alias.verb(x, ...)` or `verb(x, ...)` of the module */
        if (!ident(home, j) || !punct(home, j + 1, "(")) continue;
        if (j > fn->body_first && punct(home, j - 1, "->")) continue;
        const KAst *callee = home;
        if (j > fn->body_first + 1 && punct(home, j - 1, ".")) {
            if (!ident(home, j - 2)) continue;
            const KModule *m = module_alias(home->symbols, tok(home, j - 2));
            callee = m ? ast_of(c, m) : NULL;
            if (!callee) continue;
        }
        size_t close = close_paren(home, j + 1, fn->body_end);
        if (close == SIZE_MAX) continue;
        Range first = {0, 0};
        size_t arity = call_args(home, j + 1, close, &first);
        if (!arity) continue;
        while (first.first < first.end && (punct(home, first.first, "&") || punct(home, first.first, "*"))) first.first++;
        if (first.end != first.first + 1 || !ident(home, first.first)) continue;
        keel_slice_char text;
        if (!declared_type(home, fn, tok(home, first.first), j, &text)) continue;
        int saved = c->type_count;
        int obj = kt_from_home(c, home, text, t);
        if (obj >= 0 && kt(c, obj)->kind == KT_MODIFIER && kt(c, obj)->module && ast_of(c, kt(c, obj)->module) == callee) {
            Sig sig = find_verb(callee, tok(home, j), arity);
            const char *inner = NULL;
            if (sig.found && !sig.p[0].is_protocol && !verb_available(c, callee, sig.fn, obj, &inner)) {
                put_str(why, "calls ");
                put_str(why, kt(c, obj)->symbol);
                put_str(why, ".");
                put(why, tok(home, j).ptr, tok(home, j).len);
                put_str(why, "/");
                put_uint(why, arity);
                c->type_count = saved;
                return false;
            }
        }
        c->type_count = saved;
    }
    return true;
}

static bool verb_available(Ctx *c, const KAst *home, const KAstNode *fn, int t, const char **cause) {
    const char *inst = kt_symbol(c, t);
    if (!*inst) return true;
    for (size_t i = 0; i < c->surface_count; i++) {
        Surface *s = &c->surface[i];
        if (s->fn != fn || strcmp(s->inst, inst)) continue;
        *cause = s->cause;
        return s->state != 2;               /* a verb still being computed is a cycle: no cause on it */
    }
    if (c->surface_count == K_SURFACE_MAX) return true;
    size_t at = c->surface_count++;
    Surface *s = &c->surface[at];
    memset(s, 0, sizeof *s);
    strncpy(s->inst, inst, sizeof s->inst - 1);
    s->fn = fn;
    Text why = { .ok = true };
    bool ok = compute_surface(c, home, fn, t, &why);
    s = &c->surface[at];
    s->state = ok ? 1 : 2;
    size_t n = why.n < sizeof s->cause - 1 ? why.n : sizeof s->cause - 1;
    memcpy(s->cause, why.s, n);
    s->cause[n] = 0;
    *cause = s->cause;
    return ok;
}

/* the instances inside type `t`: itself and its arguments, at any depth */
static size_t instances_in(Ctx *c, int t, int *out, size_t n, size_t cap, int depth) {
    if (t < 0 || depth > 8) return n;
    const KType *k = kt(c, t);
    if (k->kind == KT_MODIFIER && *k->symbol && n < cap) out[n++] = t;
    if (k->kind == KT_MODIFIER)
        for (int i = 0; i < k->argc; i++) n = instances_in(c, k->arg[i], out, n, cap, depth + 1);
    return n;
}

/* a copy of the type tree `t` at the top of the pool */
static int kt_deep(Ctx *c, const KType *nodes, int t) {
    int copy = kt_new(c);
    if (copy < 0) return -1;
    c->types[copy] = nodes[t];
    if (nodes[t].kind == KT_MODIFIER)
        for (int i = 0; i < nodes[t].argc; i++) {
            int arg = kt_deep(c, nodes, nodes[t].arg[i]);
            if (arg < 0) return -1;
            c->types[copy].arg[i] = arg;
        }
    if (nodes[t].kind == KT_ARRAY && nodes[t].elem >= 0) c->types[copy].elem = kt_deep(c, nodes, nodes[t].elem);
    return copy;
}

#define K_CLOSURE_MAX 64

static void put_line(Ctx *c, const Text *t) {
    KAst *a = c->ast;
    if (!t->ok || t->n + 1 > a->closure_text.cap - a->closure_text.len) {
        if (!c->failed) {
            KToken at = tok(a, a->module);
            k_diag_emit(c->diag, K_DIAG_CAPACITY, at, (KDiagArgs){{at}});
            c->failed = true;
        }
        return;
    }
    memcpy(a->closure_text.ptr + a->closure_text.len, t->s, t->n);
    a->closure_text.len += t->n;
    a->closure_text.ptr[a->closure_text.len++] = '\n';
}

/* Parser §5, codegen §7.3: an instance comes out whole, so the instances its
   available verbs mention come with it, until nothing new appears. The seeds
   are the instances the module writes (`inst`); the dump prints the ones the
   closure adds, sorted by symbol (codegen §9), and the verbs each instance
   loses, with the cause (cgen-tool §5.2). */
static void close_instances(Ctx *c) {
    KAst *a = c->ast;
    c->type_count = 0;
    c->nest = 1;                            /* no resolution resets the pool under the closure */
    int inst[K_CLOSURE_MAX];
    char origin[K_CLOSURE_MAX][K_SYMBOL_MAX + 64];
    size_t count = 0, direct = 0;
    for (size_t i = 0; i < a->instances.len && count < K_CLOSURE_MAX; i++) {
        const KInstanceUse *u = keel_buffer_KInstanceUse_ptr(&a->instances, i);
        int t = kt_from_text(c, a->symbols, range_text(a, (Range){ u->first, u->end }));
        if (t < 0 || kt(c, t)->kind != KT_MODIFIER || !*kt(c, t)->symbol) continue;
        inst[count] = t;
        origin[count++][0] = 0;
    }
    direct = count;
    for (size_t q = 0; q < count && !c->failed; q++) {
        const KType *k = kt(c, inst[q]);
        const KAst *home = k->module ? ast_of(c, k->module) : NULL;
        if (!home || home == a) continue;
        for (size_t i = 0; i < home->nodes.len; i++) {
            const KAstNode *fn = node_at(home, i);
            if (fn->kind != K_AST_FUNCTION) continue;
            Param p[K_PARAMS_MAX];
            size_t np = params_of(home, fn, p, K_PARAMS_MAX);
            if (np == SIZE_MAX || (np && p[0].is_protocol)) continue;     /* of the module, not of an instance */
            const char *cause = NULL;
            if (!verb_available(c, home, fn, inst[q], &cause)) continue;
            int pinned = c->type_count;
            int found[16];
            size_t nf = instances_in(c, ret_type(c, home, fn, inst[q]), found, 0, 16, 0);
            for (size_t m = 0; m < np; m++) {
                if (p[m].is_type || p[m].is_array) continue;
                Range r = param_type(home, &p[m]);
                if (r.end > r.first) nf = instances_in(c, kt_from_home(c, home, range_text(home, r), inst[q]), found, nf, 16, 0);
            }
            KType scratch[64];
            int roots[16];
            size_t ns = 0, nr = 0;
            for (size_t f = 0; f < nf; f++) {
                bool known = false;
                for (size_t e = 0; e < count; e++) if (!strcmp(kt(c, inst[e])->symbol, kt(c, found[f])->symbol)) known = true;
                for (size_t e = 0; e < nr; e++) if (!strcmp(scratch[roots[e]].symbol, kt(c, found[f])->symbol)) known = true;
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
                int t = kt_deep(c, scratch, roots[e]);
                if (t < 0) break;
                Text o = { .ok = true };
                put_str(&o, k->symbol);
                put_str(&o, ".");
                put(&o, tok(home, fn->name_first).ptr, tok(home, fn->name_first).len);
                put_str(&o, "/");
                put_uint(&o, np);
                size_t len = o.n < sizeof origin[0] - 1 ? o.n : sizeof origin[0] - 1;
                memcpy(origin[count], o.s, len);
                origin[count][len] = 0;
                inst[count++] = t;
            }
        }
    }
    /* closure lines, by symbol */
    size_t order[K_CLOSURE_MAX], n = 0;
    for (size_t q = direct; q < count; q++) order[n++] = q;
    for (size_t x = 1; x < n; x++)
        for (size_t y = x; y > 0 && strcmp(kt(c, inst[order[y - 1]])->symbol, kt(c, inst[order[y]])->symbol) > 0; y--) {
            size_t tmp = order[y]; order[y] = order[y - 1]; order[y - 1] = tmp;
        }
    for (size_t x = 0; x < n; x++) {
        Text line = { .ok = true };
        put_str(&line, "closure\t");
        put_str(&line, kt(c, inst[order[x]])->symbol);
        put_str(&line, "\t");
        put_str(&line, origin[order[x]]);
        put_line(c, &line);
    }
    /* the verbs each instance loses: the written ones first, then the closure */
    size_t all[K_CLOSURE_MAX], na = 0;
    for (size_t q = 0; q < direct; q++) all[na++] = q;
    for (size_t x = 0; x < n; x++) all[na++] = order[x];
    for (size_t x = 0; x < na; x++) {
        const KType *k = kt(c, inst[all[x]]);
        const KAst *home = k->module ? ast_of(c, k->module) : NULL;
        if (!home || home == a) continue;
        for (size_t i = 0; i < home->nodes.len; i++) {
            const KAstNode *fn = node_at(home, i);
            if (fn->kind != K_AST_FUNCTION) continue;
            Param p[K_PARAMS_MAX];
            size_t np = params_of(home, fn, p, K_PARAMS_MAX);
            if (np == SIZE_MAX || (np && p[0].is_protocol)) continue;
            const char *cause = NULL;
            if (verb_available(c, home, fn, inst[all[x]], &cause)) continue;
            Text line = { .ok = true };
            put_str(&line, "unavailable\t");
            put_str(&line, k->symbol);
            put_str(&line, "\t");
            put(&line, tok(home, fn->name_first).ptr, tok(home, fn->name_first).len);
            put_str(&line, "/");
            put_uint(&line, np);
            put_str(&line, "\t");
            put_str(&line, cause ? cause : "");
            put_line(c, &line);
        }
    }
    c->nest = 0;
}

/* ---- instance (spec §4.3) ---------------------------------------------- */

/* `instance M.mod args;`: the type is a modifier of a generic module
   (instance-not-modifier), and that module has a body to place: a generic
   that is `inline` all through needs none (redundant-instance). */
static void instance_decl(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    keel_slice_char text = range_text(a, (Range){ n->name_first, n->name_end });
    KLexer lexer;
    TKPpKind pp;
    KSpecifier spec;
    KToken next = {0};
    KToken first = enter(a, n->name_first, &lexer, &pp);
    if (!k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) || spec.kind != K_SPEC_MODIFIER ||
        (next.len && next.ptr < text.ptr + text.len)) {
        diag3(c, K_DIAG_INSTANCE_NOT_MODIFIER, tok(a, n->name_first), text, none, none);
        return;
    }
    check_arguments(c, &spec, tok(a, n->name_first));
    const KSymbol *m = k_symbol_resolve(a->symbols, spec.modifier_name);
    const KAst *home = m && m->origin ? ast_of(c, m->origin) : NULL;
    if (!home) return;
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *fn = node_at(home, i);
        if ((fn->kind == K_AST_FUNCTION && !fn->is_inline) || fn->kind == K_AST_VARIABLE) return;
    }
    diag3(c, K_DIAG_REDUNDANT_INSTANCE, tok(a, n->first), text, none, none);
}

/* Spec §4.3, rule 20: in a generic module, a declaration that mentions no
   parameter and no modifier belongs to the module and is emitted once, so it
   is a type, a `constexpr` or an `inline` function — never one out of line
   nor a variable (nonparametric-out-of-line). */
static void check_generic(Ctx *c) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    const KAstNode *m = module_node(a);
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = node_at(a, i);
        if (!((n->kind == K_AST_FUNCTION && !n->is_inline) || n->kind == K_AST_VARIABLE)) continue;
        bool mentions = false;
        size_t end = n->body_first != SIZE_MAX ? n->body_first : n->end;
        for (size_t j = n->first; j < end && !mentions; j++) {
            if (!ident(a, j)) continue;
            for (size_t k = m->dim_first; k < m->type_end; k++)
                if (ident(a, k) && k_symtab_same_name(tok(a, j), tok(a, k))) mentions = true;
            for (size_t k = 0; k < a->nodes.len && !mentions; k++) {
                const KAstNode *d = node_at(a, k);
                if (d->kind == K_AST_MODIFIER && k_symtab_same_name(tok(a, j), tok(a, d->name_first))) mentions = true;
            }
        }
        if (!mentions)
            diag3(c, K_DIAG_NONPARAMETRIC_OUT_OF_LINE, tok(a, n->name_first), tok(a, n->name_first), none, none);
    }
}

/* a modifier is named `instance` (modifier-named-instance), or stands in a
   module with no parameters (modifier-outside-generic) */
static void check_modifiers(Ctx *c) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = node_at(a, i);
        if (n->kind != K_AST_MODIFIER) continue;
        KToken name = tok(a, n->name_first);
        if (k_token_spelled(name, "instance")) diag3(c, K_DIAG_MODIFIER_NAMED_INSTANCE, name, name, none, none);
        if (!generic(a)) diag3(c, K_DIAG_MODIFIER_OUTSIDE_GENERIC, name, name, none, none);
    }
}

/* ---- names across modules (spec §2.5, §4.1) ---------------------------- */

static bool module_generic(const KModule *m) {
    if (!m || !m->ast) return false;
    const KAstNode *n = node_at(m->ast, m->ast->module);
    return n->dim_first != n->type_end;
}

/* the canonical names a symbol takes in C: its own, and each value of a set */
static size_t canonical_names(const KSymbol *sym, const KAst *own, char out[][K_SYMBOL_MAX], size_t cap) {
    char buf[K_SYMBOL_MAX];
    size_t n = k_mangle_symbol(sym, (keel_slice_char){ sizeof buf, buf });
    if (n >= sizeof buf || !cap) return 0;
    memcpy(out[0], buf, n);
    out[0][n] = 0;
    size_t count = 1;
    KTagsDecl tags;
    const KAst *home = sym->origin && sym->origin->ast ? sym->origin->ast : own;    /* the file's own: not retained yet */
    if (sym->kind == K_SYM_TAGS && home && tags_decl(home, sym->name, &tags))
        for (size_t k = 0; k < tags.item_count && count < cap; k++) {
            if (n + 1 + tags.items[k].len >= K_SYMBOL_MAX) continue;
            memcpy(out[count], buf, n);
            out[count][n] = '_';
            memcpy(out[count] + n + 1, tags.items[k].ptr, tags.items[k].len);
            out[count][n + 1 + tags.items[k].len] = 0;
            count++;
        }
    return count;
}

/* Spec §4.1, rule 9: the module's exported symbols against those of the
   transitive closure of its imports, by canonical name (symbol-collision). */
static void check_collisions(Ctx *c) {
    KAst *a = c->ast;
    const KAstNode *mod = module_node(a);
    keel_slice_char self = range_text(a, (Range){ mod->name_first, mod->name_end });
    const KModule *seen[64];
    size_t ns = 0;
    for (size_t i = 0; i < a->symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(a->symbols, i);
        if (s->kind == K_SYM_MODULE && s->origin && ns < 64) {
            bool dup = false;
            for (size_t k = 0; k < ns; k++) if (seen[k] == s->origin) dup = true;
            if (!dup) seen[ns++] = s->origin;
        }
    }
    for (size_t q = 0; q < ns; q++) {                          /* the closure */
        const KAst *ma = seen[q]->ast;
        if (!ma || !ma->symbols) continue;
        for (size_t i = 0; i < ma->symbols->len && ns < 64; i++) {
            const KSymbol *s = keel_buffer_KSymbol_ptr(ma->symbols, i);
            if (s->kind != K_SYM_MODULE || !s->origin) continue;
            bool dup = false;
            for (size_t k = 0; k < ns; k++) if (seen[k] == s->origin) dup = true;
            if (!dup) seen[ns++] = s->origin;
        }
    }
    for (size_t i = 0; i < a->symbols->len; i++) {
        const KSymbol *own = keel_buffer_KSymbol_ptr(a->symbols, i);
        if (!own->origin || !k_symtab_same_name(own->origin->name, self) || own->kind == K_SYM_MODULE ||
            own->kind == K_SYM_MODIFIER || own->kind == K_SYM_PROTOCOL)
            continue;
        char mine[K_TAGS_MAX_ITEMS + 1][K_SYMBOL_MAX];
        size_t nm = canonical_names(own, a, mine, K_TAGS_MAX_ITEMS + 1);
        for (size_t q = 0; q < ns; q++) {
            const KModule *m = seen[q];
            if (module_generic(m) || k_symtab_same_name(m->name, self)) continue;
            for (size_t k = 0; k < m->symbol_count; k++) {
                const KSymbol *other = &m->symbols[k];
                if (other->kind == K_SYM_MODIFIER || other->kind == K_SYM_PROTOCOL) continue;
                char theirs[K_TAGS_MAX_ITEMS + 1][K_SYMBOL_MAX];
                size_t nt = canonical_names(other, NULL, theirs, K_TAGS_MAX_ITEMS + 1);
                for (size_t x = 0; x < nm; x++)
                    for (size_t y = 0; y < nt; y++)
                        if (!strcmp(mine[x], theirs[y])) {
                            Text name = { .ok = true };
                            put_str(&name, mine[x]);
                            KToken at = own->name;
                            for (size_t j = 0; j < a->nodes.len; j++) {
                                const KAstNode *d = node_at(a, j);
                                if (d->name_first != d->name_end && k_symtab_same_name(tok(a, d->name_first), own->name))
                                    { at = tok(a, d->name_first); break; }
                            }
                            diag3(c, K_DIAG_SYMBOL_COLLISION, at, keep(c, &name), own->name, m->name);
                        }
            }
        }
    }
}

/* Spec §2.5: an alias of a module and a type of another origin spelled
   alike in the file (alias-type-collision). The alias `outcome` and the
   modifier `outcome` that `types` injects share an origin, and are fine. */
static void check_alias_types(Ctx *c) {
    KAst *a = c->ast;
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = node_at(a, i);
        if (n->kind != K_AST_IMPORT || n->alias == SIZE_MAX) continue;
        KToken alias = tok(a, n->alias);
        const KSymbol *m = NULL;
        for (size_t k = 0; k < a->symbols->len; k++) {
            const KSymbol *s = keel_buffer_KSymbol_ptr(a->symbols, k);
            if (s->kind == K_SYM_MODULE && k_symtab_same_name(s->name, alias)) { m = s; break; }
        }
        if (!m) continue;
        for (size_t k = 0; k < a->symbols->len; k++) {
            const KSymbol *s = keel_buffer_KSymbol_ptr(a->symbols, k);
            bool typeish = s->kind == K_SYM_TYPE || s->kind == K_SYM_MODIFIER || s->kind == K_SYM_TAGS ||
                           s->kind == K_SYM_PROTOCOL;
            if (typeish && s->origin != m->origin && k_symtab_same_name(s->name, alias)) {
                diag3(c, K_DIAG_ALIAS_TYPE_COLLISION, alias, alias, s->origin ? s->origin->name : k_diag_text("this file"),
                      m->origin ? m->origin->name : alias);
                break;
            }
        }
    }
}

/* ---- declarations (passage 2: spec §2.5, §4.1, §4.2, §4.3, §4.9) ------- */

/* a declared name: in the reserved spaces (reserved-name), or a contextual
   word of keel (keel-name-shadowed; roles are not in the list, and the verbs
   `win` and `fail` and the module names `array` and `parallel` are let
   through, spec §6.2) */
static void check_name(Ctx *c, KToken name, bool top) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (k_token_starts_with(name, "keel_") || k_token_starts_with(name, "KEEL_"))
        diag3(c, K_DIAG_RESERVED_NAME, name, name, none, none);
    if (k_token_is_keel_word(name) && !k_token_spelled(name, "win") && !k_token_spelled(name, "fail") &&
        !k_token_spelled(name, "array") && !k_token_spelled(name, "parallel"))
        diag3(c, K_DIAG_KEEL_NAME_SHADOWED, name, name, none, none);
    (void)top;
    /* shadowed-injected-name waits on the base: keel.buffer declares its own
       `cursor` and imports keel.slice, which injects one (see the README) */
}

/* whether the type a specifier names is `byref`: a modifier declared
   `modifier X byref`, a `typedef ... X byref;`, or `arena` (spec §4.3, rule 17) */
static bool spec_byref(const KAst *a, const KSpecifier *spec) {
    if (is_arena(a, spec)) return true;
    keel_slice_char name = spec->kind == K_SPEC_MODIFIER ? spec->modifier_name : spec->type_name;
    const KSymbol *sym = k_symbol_resolve(a->symbols, name);
    const KAst *home = sym && sym->origin && sym->origin->ast ? sym->origin->ast : a;
    if (!sym) return false;
    for (size_t i = 0; i < home->nodes.len; i++) {
        const KAstNode *n = node_at(home, i);
        if (n->name_first == n->name_end || !k_symtab_same_name(tok(home, n->name_first), sym->name)) continue;
        if (n->kind == K_AST_MODIFIER) return word(home, n->name_end, "byref");
        if (n->kind == K_AST_TYPE)
            for (size_t j = n->name_end; j < n->end; j++) if (word(home, j, "byref")) return true;
    }
    return false;
}

/* the dimensions and the `tags` arguments of a modifier written in `spec`
   (spec §4.3, rules 4 and 5): a `dim` is a known decimal of at least 1
   (nonconstant-dim, dim-below-one), and a `tags` argument names a set
   (undeclared-tags) */
static void check_arguments(Ctx *c, const KSpecifier *spec, KToken at) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (spec->kind != K_SPEC_MODIFIER) return;
    for (size_t d = 0; d < spec->dim_count; d++) {
        keel_slice_char t = spec->dims[d];
        while (t.len && t.ptr[0] == ' ') { t.ptr++; t.len--; }
        while (t.len && t.ptr[t.len - 1] == ' ') t.len--;
        long v = -1;
        bool known = digits(t, &v);
        if (!known) {
            const Local *l = find_local(c, t);
            const KSymbol *sym = l ? NULL : k_symbol_resolve(a->symbols, t);
            known = l ? l->is_constexpr && digits(l->value, &v) : sym && sym->kind == K_SYM_CONSTANT && digits(sym->value, &v);
        }
        if (!known) diag3(c, K_DIAG_NONCONSTANT_DIM, at, t, spec->text, none);
        else if (v < 1) diag3(c, K_DIAG_DIM_BELOW_ONE, at, t, spec->text, none);
    }
    const KSymbol *m = k_symbol_resolve(a->symbols, spec->modifier_name);
    const KAst *home = m && m->origin ? m->origin->ast : NULL;
    if (!home) return;
    const KAstNode *mn = module_node(home);
    size_t tags = 0;
    for (size_t j = mn->tags_first; j < mn->tags_end; j++) if (ident(home, j)) tags++;
    for (size_t k = 0; k < tags && k < spec->arg_count; k++) {
        const KSymbol *s = k_symbol_resolve(a->symbols, spec->args[k]);
        if (!s || s->kind != K_SYM_TAGS) diag3(c, K_DIAG_UNDECLARED_TAGS, at, spec->args[k], spec->text, none);
    }
}

static void check_declarations(Ctx *c) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    const KAstNode *mod = module_node(a);
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = node_at(a, i);
        bool named = n->name_first != n->name_end;
        /* visibility and storage (spec §4.1) */
        size_t name_at = named ? n->name_first : n->end;
        bool has_static = false, has_inline = false, has_pub = word(a, n->first, "pub");
        bool has_typedef = false;
        for (size_t j = n->first; j < name_at && j < n->end; j++) {
            if (word(a, j, "static")) has_static = true;
            if (word(a, j, "inline")) has_inline = true;
            if (word(a, j, "typedef")) has_typedef = true;
        }
        if (n->kind == K_AST_FUNCTION || n->kind == K_AST_VARIABLE) {
            if (has_pub && has_static && !has_inline) diag3(c, K_DIAG_PUB_STATIC, tok(a, n->first), none, none, none);
            if (word(a, n->first, "static") && has_inline) diag3(c, K_DIAG_INLINE_WITHOUT_VISIBILITY, tok(a, n->first), none, none, none);
        }
        if ((n->kind == K_AST_TYPE || has_typedef) && has_static) diag3(c, K_DIAG_STATIC_ON_TYPE, tok(a, n->first), none, none, none);
        /* sets of tags (spec §4.9) */
        if (n->kind == K_AST_TAGS) {
            if (!named) { diag3(c, K_DIAG_UNNAMED_TAGS, tok(a, n->first), none, none, none); continue; }
            KTagsDecl t;
            if (tags_decl(a, tok(a, n->name_first), &t)) {
                if (!t.item_count) diag3(c, K_DIAG_EMPTY_TAGS, tok(a, n->name_first), tok(a, n->name_first), none, none);
                if (t.has_values && !t.all_values)
                    diag3(c, K_DIAG_PARTIAL_TAG_VALUES, tok(a, n->name_first), tok(a, n->name_first), none, none);
                for (size_t k = 0; k < t.item_count; k++) {
                    long v;
                    keel_slice_char val = t.values[k];
                    if (!val.len) continue;
                    if (val.ptr[0] == '-') { val.ptr++; val.len--; while (val.len && val.ptr[0] == ' ') { val.ptr++; val.len--; } }
                    const KSymbol *sym = digits(val, &v) ? NULL : k_symbol_resolve(a->symbols, val);
                    if (!digits(val, &v) && !(sym && sym->kind == K_SYM_CONSTANT))
                        diag3(c, K_DIAG_NONCONSTANT_TAG_VALUE, t.items[k], t.values[k], t.items[k], none);
                }
            }
            for (size_t k = 0; k < i; k++) {
                const KAstNode *o = node_at(a, k);
                if (o->kind == K_AST_TAGS && o->name_first != o->name_end &&
                    k_symtab_same_name(tok(a, o->name_first), tok(a, n->name_first))) {
                    diag3(c, K_DIAG_DUPLICATE_TAGS_NAME, tok(a, n->name_first), tok(a, n->name_first), none, none);
                    break;
                }
            }
        }
        if (!named || n->kind == K_AST_IMPORT || n->kind == K_AST_IMPORT_C || n->kind == K_AST_MODULE ||
            n->kind == K_AST_INSTANCE)
            continue;
        KToken name = tok(a, n->name_first);
        check_name(c, name, true);
        /* the module's parameters are not redeclared (spec §4.3) */
        for (size_t k = mod->dim_first; k < mod->type_end; k++)
            if (ident(a, k) && k_symtab_same_name(tok(a, k), name))
                diag3(c, K_DIAG_PARAMETER_NAME_REUSE, name, name, none, none);
        if (n->kind != K_AST_FUNCTION) continue;
        Param p[K_PARAMS_MAX];
        size_t np = params_of(a, n, p, K_PARAMS_MAX);
        for (size_t k = 0; np != SIZE_MAX && k < np; k++) {
            if (p[k].end <= p[k].first) continue;
            KToken pn = tok(a, p[k].end - 1);
            if (ident(a, p[k].end - 1) && !p[k].is_type) {      /* `type T` selects the instance (§4.4) */
                check_name(c, pn, false);
                for (size_t m = mod->dim_first; m < mod->type_end; m++)
                    if (ident(a, m) && k_symtab_same_name(tok(a, m), pn))
                        diag3(c, K_DIAG_PARAMETER_NAME_REUSE, pn, pn, none, none);
            }
            /* a `byref` instance by value (spec §4.3, rule 17) */
            Range r = param_type(a, &p[k]);
            if (p[k].pointer || p[k].is_type || p[k].is_array || r.end <= r.first) continue;
            KLexer lexer;
            TKPpKind pp;
            KSpecifier spec;
            KToken next = {0};
            KToken first = enter(a, r.first, &lexer, &pp);
            if (k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE &&
                spec_byref(a, &spec))
                diag3(c, K_DIAG_BYREF_PARAM, pn, pn, spec.text, none);
        }
    }
    /* the canonical names of the module's own declarations (spec §4.2) */
    keel_slice_char self = range_text(a, (Range){ mod->name_first, mod->name_end });
    char names[96][K_SYMBOL_MAX];
    keel_slice_char whose[96];
    size_t nn = 0;
    for (size_t i = 0; i < a->symbols->len && nn < 96; i++) {
        const KSymbol *own = keel_buffer_KSymbol_ptr(a->symbols, i);
        if (!own->origin || !k_symtab_same_name(own->origin->name, self) || own->kind == K_SYM_MODULE) continue;
        char mine[K_TAGS_MAX_ITEMS + 1][K_SYMBOL_MAX];
        size_t nm = canonical_names(own, a, mine, K_TAGS_MAX_ITEMS + 1);
        bool told = false;
        for (size_t x = 0; x < nm && nn < 96; x++) {
            for (size_t y = 0; y < nn && !told; y++)
                if (!strcmp(names[y], mine[x]) && !k_symtab_same_name(whose[y], own->name)) {
                    Text t = { .ok = true };
                    put_str(&t, mine[x]);
                    diag3(c, K_DIAG_CANONICAL_NAME_COLLISION, own->name, keep(c, &t), whose[y], own->name);
                    told = true;                        /* once per declaration */
                }
            memcpy(names[nn], mine[x], K_SYMBOL_MAX);
            whose[nn++] = own->name;
        }
    }
}

/* `constexpr T NAME = value;` (spec §4.2): the name is the token before the
   only `=` at the top (constexpr-name-missing), and the declaration is a
   scalar: no array declarator, no braces around the value (nonscalar-constexpr) */
static void check_constexpr(Ctx *c, size_t kw, size_t end) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t eq = SIZE_MAX, depth = 0;
    for (size_t j = kw + 1; j < end && !(depth == 0 && punct(a, j, ";")); j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) depth--;
        else if (!depth && punct(a, j, "=")) { eq = j; break; }
    }
    if (eq == SIZE_MAX) return;
    if (punct(a, eq - 1, "]") || punct(a, eq + 1, "{")) {
        diag3(c, K_DIAG_NONSCALAR_CONSTEXPR, tok(a, kw), none, none, none);
        return;
    }
    if (!ident(a, eq - 1)) diag3(c, K_DIAG_CONSTEXPR_NAME_MISSING, tok(a, eq - 1), tok(a, eq - 1), none, none);
}

/* ---- extern_c and main (spec §4.1) ------------------------------------ */

/* `priv extern_c [type_h]` asks for the interface and the implementation at
   once (type-layer-on-priv-extern-c), and `main` is not defined inside one
   (main-in-extern-c): it is a function of the module, which takes its prefix. */
static void check_extern_c(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    size_t kw = n->first;
    while (kw < n->end && !word(a, kw, "extern_c")) kw++;
    if (!n->is_public)
        for (size_t j = kw; j < n->end && j < n->body_first; j++)
            if (word(a, j, "type_h")) { diag3(c, K_DIAG_TYPE_LAYER_ON_PRIV_EXTERN_C, tok(a, kw), none, none, none); break; }
    if (n->body_first == SIZE_MAX) return;
    size_t depth = 0, top = punct(a, n->body_first, "{") ? 1 : 0;   /* the body's span may hold its braces */
    for (size_t j = n->body_first; j < n->body_end; j++) {
        if (punct(a, j, "{") || punct(a, j, "(")) depth++;
        else if (punct(a, j, "}") || punct(a, j, ")")) depth--;
        else if (depth == top && word(a, j, "main") && punct(a, j + 1, "(")) {
            size_t cp = close_paren(a, j + 1, n->body_end);
            if (cp != SIZE_MAX && punct(a, cp + 1, "{")) diag3(c, K_DIAG_MAIN_IN_EXTERN_C, tok(a, j), none, none, none);
        }
    }
}

/* `main` is public (private-main) and has one of the two forms of C:
   `int main(void)` or `int main(int argc, char *argv[])` (invalid-main-signature) */
static void check_main(Ctx *c, const KAstNode *fn) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    KToken name = tok(a, fn->name_first);
    if (!k_token_spelled(name, "main")) return;
    if (!fn->is_public) diag3(c, K_DIAG_PRIVATE_MAIN, name, none, none, none);
    size_t s = fn->first;
    while (s < fn->name_first && (word(a, s, "pub") || word(a, s, "priv"))) s++;
    bool ok = s + 1 == fn->name_first && word(a, s, "int") && punct(a, fn->name_first + 1, "(");
    size_t open = fn->name_first + 1, close = ok ? close_paren(a, open, fn->end) : SIZE_MAX;
    if (close == SIZE_MAX) ok = false;
    else if (close == open + 1 || (close == open + 2 && word(a, open + 1, "void"))) { /* () and (void) */ }
    else {
        /* int NAME , char * NAME [ ]   or   int NAME , char * * NAME */
        size_t j = open + 1;
        ok = word(a, j, "int") && ident(a, j + 1) && punct(a, j + 2, ",") && word(a, j + 3, "char") && punct(a, j + 4, "*");
        if (ok && punct(a, j + 5, "*")) ok = ident(a, j + 6) && j + 7 == close;
        else if (ok) ok = ident(a, j + 5) && punct(a, j + 6, "[") && punct(a, j + 7, "]") && j + 8 == close;
    }
    if (!ok) diag3(c, K_DIAG_INVALID_MAIN_SIGNATURE, name, none, none, none);
}

/* What the v0 grammar recognizes in a signature only to refuse or place it:
   `keel_code` and the mold's `dim` parameter are v1 (spec §4.13), and a role
   stands before a parameter's type, or `child` before the return type
   (spec §4.12). */
static void check_signature(Ctx *c, const KAstNode *fn) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    for (size_t j = fn->first; j < fn->name_first; j++) {
        if (word(a, j, "keel_code")) diag3(c, K_DIAG_NOT_IN_V0, tok(a, j), tok(a, j), none, none);
        KRole r = k_token_role(tok(a, j));
        if (r != K_ROLE_NONE && r != K_ROLE_CHILD) diag3(c, K_DIAG_ROLE_POSITION, tok(a, j), tok(a, j), none, none);
    }
    Param p[K_PARAMS_MAX];
    size_t n = params_of(a, fn, p, K_PARAMS_MAX);
    for (size_t k = 0; n != SIZE_MAX && k < n; k++) {
        size_t lead = p[k].role != K_ROLE_NONE ? p[k].first + 1 : p[k].first;
        if (word(a, lead, "keel_code") || (word(a, lead, "dim") && lead + 2 == p[k].end && ident(a, lead + 1)))
            diag3(c, K_DIAG_NOT_IN_V0, tok(a, lead), tok(a, lead), none, none);
        if (p[k].role != K_ROLE_NONE && p[k].is_type)
            diag3(c, K_DIAG_ROLE_POSITION, tok(a, p[k].first), tok(a, p[k].first), none, none);
    }
}

bool k_collect_islands(KAst *a, KDiagnosticSink *diag) {
    a->islands.len = 0;
    a->island_text.len = 0;
    a->closure_text.len = 0;
    if (!a->symbols) return true;
    static Surface surface[K_SURFACE_MAX];
    Ctx ctx = { .ast = a, .diag = diag, .surface = surface };
    check_modifiers(&ctx);
    check_declarations(&ctx);
    if (generic(a)) {
        check_generic(&ctx);
        return true;
    }
    check_collisions(&ctx);
    check_alias_types(&ctx);
    for (size_t i = 0; i < a->nodes.len; i++) {
        const KAstNode *n = node_at(a, i);
        if (n->kind == K_AST_INSTANCE) instance_decl(&ctx, n);
        if (n->kind == K_AST_EXTERN_C) check_extern_c(&ctx, n);
        if (n->kind == K_AST_FUNCTION && n->name_first != n->name_end) check_main(&ctx, n);
        if (n->kind == K_AST_CONSTEXPR) {
            size_t kw = n->first;
            while (kw < n->end && !word(a, kw, "constexpr")) kw++;
            check_constexpr(&ctx, kw, n->end);
        }
    }
    /* what the file declares at file scope is known to every function, wherever
       it stands in the file: read those declarations first, printing nothing */
    ctx.quiet = true;
    size_t seen = 0;
    for (size_t i = 0; i < a->nodes.len && !ctx.failed; i++) {
        const KAstNode *n = node_at(a, i);
        if (n->kind != K_AST_VARIABLE || n->first < seen) continue;
        seen = n->end;
        walk(&ctx, n);
        for (size_t k = ctx.global_count; k < ctx.local_count && ctx.global_count < K_LOCALS_MAX; k++)
            if (ctx.locals[k].depth == 0) {
                ctx.globals[ctx.global_count] = ctx.locals[k];
                ctx.globals[ctx.global_count++].global = true;
            }
    }
    ctx.quiet = false;
    for (size_t i = 0; i < a->nodes.len; i++)
        if (node_at(a, i)->kind == K_AST_FUNCTION) check_signature(&ctx, node_at(a, i));
    for (size_t i = 0; i < a->nodes.len && !ctx.failed; i++)
        if (node_at(a, i)->kind == K_AST_TYPE) {
            ctx.local_count = ctx.global_count;
            memcpy(ctx.locals, ctx.globals, ctx.global_count * sizeof *ctx.globals);
            extent_island(&ctx, node_at(a, i));
        }
    size_t covered = 0;
    for (size_t i = 0; i < a->nodes.len && !ctx.failed; i++) {
        const KAstNode *n = node_at(a, i);
        if (n->kind != K_AST_FUNCTION && n->kind != K_AST_VARIABLE) continue;
        if (n->first < covered) continue;       /* declarators of one declaration share its span */
        covered = n->end;
        walk(&ctx, n);
    }
    if (!ctx.failed) close_instances(&ctx);
    return !ctx.failed;
}
