/* Pass 3 (parser-design §3), stages 4a and 4b: type, name, call,
 * ref and implicit-init islands, then array, array-index, index and
 * range-index, read from the token stream of each function and variable
 * declaration — signature and body alike.
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

#define K_TYPES_MAX 64
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
} Ctx;

typedef struct { char s[K_DETAIL_MAX]; size_t n; bool ok; } Text;

typedef struct { size_t first, end; } Range;      /* tokens [first, end) */

typedef struct {
    bool is_type;               /* `type X` */
    bool is_array;              /* `array T v[size_t N]` */
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
            Param p = { .is_type = word(a, start, "type"), .is_array = word(a, start, "array"),
                        .first = start, .end = j };
            for (size_t k = start; k < j; k++) {
                if (punct(a, k, "*") || (punct(a, k, "[") && !p.is_array)) p.pointer = true;
                if (!p.base.len && ident(a, k) && !((p.is_type || p.is_array) && k == start)) p.base = tok(a, k);
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
        spec.kind != K_SPEC_MODIFIER)
        return kt_from_text(c, home->symbols, body);
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
                                  word(home, s, "static") || word(home, s, "extern")))
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
    /* `slice.of` is a function over `Sliceable` (spec §4.14, backend §5.19): the
       instance is named by the concrete type, and its body calls that type's
       `length` and three-parameter `as_slice`, so the island resolves the call as
       that `as_slice` to find the instance and the type of the result */
    bool proto_of = false;
    if (module && module_named(home, "keel.slice") && k_token_spelled(name, "of") && k0 && k0->kind == KT_ARRAY) {
        if (k0->rank > 1) {
            diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, tok(a, callee), written, range_text(a, args[0]), k0->dims);
            return -1;
        }
        const KModule *am = module_by_name(a->symbols, "keel.array");
        if (!am || !ast_of(c, am) || !find_verb(ast_of(c, am), k_diag_text("as_slice"), 3).found ||
            !find_verb(ast_of(c, am), k_diag_text("length"), 1).found) {
            diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, callee), range_text(a, args[0]), k_diag_text("Sliceable"), written);
            return -1;
        }
        module = am;
        home = ast_of(c, module);
        redirected = k_diag_text("as_slice");
        proto_of = true;
    } else if (module && module_named(home, "keel.slice") && k_token_spelled(name, "of") && k0 &&
               k0->kind == KT_MODIFIER && k0->module && ast_of(c, k0->module)) {
        const KAst *own = ast_of(c, k0->module);
        if (!find_verb(own, k_diag_text("as_slice"), 3).found || !find_verb(own, k_diag_text("length"), 1).found) {
            diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, tok(a, callee), range_text(a, args[0]), k_diag_text("Sliceable"), written);
            return -1;
        }
        module = k0->module;
        home = own;
        redirected = k_diag_text("as_slice");
        proto_of = true;
    }
    Sig sig = find_verb(home, redirected.len ? redirected : name, proto_of ? 3 : argc);
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
            put_str(&t, "keel_slice_of");
            if (argc > 1) put_uint(&t, argc - 1);
            put_str(&t, "_");
            put(&t, own.s, own.n);
        } else if (direct) {
            if (!*kt(c, inst)->symbol) return -1;
            put_str(&t, kt(c, inst)->symbol);
        } else if (!module_instance_prefix(c, home, inst, &t)) {
            return -1;
        }
    } else {
        put_module_prefix(&t, home);
    }
    if (!proto_of) {
        put_str(&t, "_");
        put(&t, declared.ptr, declared.len);
        if (sig.found && sig.overloaded && argc > 1) put_uint(&t, argc - 1);
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
static size_t declaration(Ctx *c, size_t i, const KSpecifier *spec, KLexer *lexer, KToken next,
                          bool signature, bool declarators) {
    KAst *a = c->ast;
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
                diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, name, path, k_diag_text("Sliceable"), group);
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
            diag3(c, K_DIAG_PROTOCOL_NOT_SATISFIED, name, path, k_diag_text("Sliceable"), text);
            return close + 1;
        }
    }
    if (adapt) put_str(&tx, " &1");
    *any = emit(c, range == SIZE_MAX ? K_ISLAND_INDEX : K_ISLAND_RANGE_INDEX, i, &tx) || *any;
    return close + 1;
}

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
        j = index_group(c, i, j, &any);
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

static void walk(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    memcpy(c->locals, c->globals, c->global_count * sizeof *c->globals);
    c->local_count = c->global_count;
    c->depth = 0;
    c->has_target = false;
    for (size_t i = n->first; i < n->end && !c->failed; i++) {
        if (!live(a, i)) continue;
        if (punct(a, i, "{")) { c->depth++; continue; }
        if (punct(a, i, "}")) {
            while (c->local_count && c->locals[c->local_count - 1].depth >= c->depth) c->local_count--;
            c->depth--;
            continue;
        }
        if (punct(a, i, ";")) { c->has_target = false; continue; }
        if (word(a, i, "constexpr") && !signature_of(n, i)) { block_constexpr(c, i); continue; }
        if (i == n->name_first || !ident(a, i)) continue;
        if (i > n->first && (punct(a, i - 1, ".") || punct(a, i - 1, "->"))) continue;

        KToken t = tok(a, i);
        check_uses(c, i);
        bool signature = signature_of(n, i);

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

bool k_collect_islands(KAst *a, KDiagnosticSink *diag) {
    a->islands.len = 0;
    a->island_text.len = 0;
    if (!a->symbols || generic(a)) return true;
    Ctx ctx = { .ast = a, .diag = diag };
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
    size_t covered = 0;
    for (size_t i = 0; i < a->nodes.len && !ctx.failed; i++) {
        const KAstNode *n = node_at(a, i);
        if (n->kind != K_AST_FUNCTION && n->kind != K_AST_VARIABLE) continue;
        if (n->first < covered) continue;       /* declarators of one declaration share its span */
        covered = n->end;
        walk(&ctx, n);
    }
    return !ctx.failed;
}
