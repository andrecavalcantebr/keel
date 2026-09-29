/* Pass 3 (parser-design §3), stages 4a and 4b: type, name, call, from-stack,
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
    bool is_ref;                /* declared with `ref` */
    bool is_constexpr;          /* a block-level `constexpr` */
    keel_slice_char value;      /* its initializer, as written */
} Local;

typedef struct {
    KAst *ast;
    KDiagnosticSink *diag;
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
            Param p = { .is_type = word(a, start, "type"), .is_array = word(a, start, "array") };
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

/* One call: `callee` is the first token of the written callee, `verb` its
   last, `open` the '(' — `module` is the qualifier's module, NULL for a call
   to the file's own function. The island is always printed, resolved or not:
   what the module does not declare goes on as the qualified call, for the C
   compiler to validate (spec §4.4, item 3). It is not printed only where this
   pass cannot name the instance: the object is a shape it does not type yet. */
static void call(Ctx *c, size_t callee, size_t verb, size_t open, const KModule *module) {
    KAst *a = c->ast;
    size_t close = close_paren(a, open, a->tokens.len);
    if (close == SIZE_MAX) return;

    Range args[K_PARAMS_MAX];
    size_t argc = 0, start = open + 1, depth = 0;
    for (size_t j = open + 1; j <= close; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (j < close && (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}"))) depth--;
        else if (j == close || (depth == 0 && punct(a, j, ","))) {
            if (j > start || argc > 0) {
                if (argc == K_PARAMS_MAX) return;
                args[argc++] = (Range){ start, j };
            }
            start = j + 1;
        }
    }

    const KAst *home = module ? module->ast : a;
    if (!home) return;
    KToken name = tok(a, verb);
    keel_slice_char written = range_text(a, (Range){ callee, verb + 1 });
    keel_slice_char redirected = {0};
    /* `slice.of` over a `buffer` is the buffer's `as_slice`, chosen by arity (backend §5.2, item 3) */
    if (module && module_named(home, "keel.slice") && k_token_spelled(name, "of") && argc) {
        const Local *object = arg_local(c, args[0]);
        const KSymbol *m = object && object->spec.kind == K_SPEC_MODIFIER && !object->rank
                         ? k_symbol_resolve(a->symbols, object->spec.modifier_name) : NULL;
        if (m && m->origin && m->origin != module && m->origin->ast && module_named(m->origin->ast, "keel.buffer")) {
            module = m->origin;
            home = module->ast;
            redirected = k_diag_text("as_slice");
        }
    }
    bool from_stack = module && module_named(home, "keel.arena") && k_token_spelled(name, "from_stack");
    Sig sig = from_stack ? find_verb(home, k_diag_text("from_array"), SIZE_MAX)
                         : find_verb(home, redirected.len ? redirected : name, argc);
    keel_slice_char declared = from_stack ? k_diag_text("from_array") : redirected.len ? redirected : name;
    /* the core's `length`, `capacity` and `dim` lower to no function (backend §5.2) */
    if (module && module_named(home, "keel")) return;
    bool gen = module && generic(home);
    keel_slice_char modifier = gen ? modifier_of(home) : (keel_slice_char){0};

    /* which parameter takes the container, which takes an `array`, and whether a
       `type` parameter selects the instance (spec §4.4, item 4) */
    size_t recv = SIZE_MAX, arr = SIZE_MAX;
    bool selection = false;
    for (size_t k = 0; sig.found && k < sig.n; k++) {
        if (sig.p[k].is_type) selection = selection || binder(home, sig.p[k].base);
        else if (sig.p[k].is_array) { if (arr == SIZE_MAX) arr = k; }
        else if (gen && recv == SIZE_MAX && k_symtab_same_name(sig.p[k].base, modifier)) recv = k;
    }
    if (!sig.found && gen && argc) recv = 0;        /* the module declares no such verb: the object is first */

    long known;
    if (from_stack && argc == 2 && !decimal_of(c, args[1], &known))
        diag3(c, K_DIAG_NONCONSTANT_ARENA_STACK, tok(a, args[1].first), range_text(a, args[1]),
              (keel_slice_char){0}, (keel_slice_char){0});

    /* slice.from over a `ref` would take a pointer keel does not vouch for (spec §5.3) */
    if (module && module_named(home, "keel.slice") && k_token_spelled(name, "from") && argc >= 2) {
        const Local *p = arg_local(c, args[1]);
        if (p && p->is_ref)
            diag3(c, K_DIAG_SLICE_FROM_REF, tok(a, args[1].first), range_text(a, args[1]), (keel_slice_char){0},
                  (keel_slice_char){0});
    }

    Text t = { .ok = true };
    put(&t, written.ptr, written.len);
    put_str(&t, "/");
    put_uint(&t, argc);
    put_str(&t, " \xe2\x86\x92 ");

    const Local *first = argc ? arg_local(c, args[0]) : NULL;

    /* buffer.of(x) needs an `array` of one dimension, to read the extent from (spec §5.3) */
    if (module && !redirected.len && module_named(home, "keel.buffer") && k_token_spelled(name, "of") &&
        argc == 1 && first && first->rank == 0) {
        diag3(c, K_DIAG_BUFFER_OF_UNKNOWN_SIZE, tok(a, callee), written, range_text(a, args[0]), (keel_slice_char){0});
        return;
    }

    /* arena.from_array(a, v) takes the extent of an `array u8` (spec §5.2) */
    if (module && module_named(home, "keel.arena") && k_token_spelled(name, "from_array") && argc == 2) {
        const Local *v = arg_local(c, args[1]);
        if (!v) return;                             /* not a symbol this pass saw declared */
        if (v->rank == 0 || !k_primitive_name(v->element) || strcmp(k_primitive_name(v->element), "u8")) {
            diag3(c, K_DIAG_ARENA_FROM_ARRAY_NOT_U8, tok(a, args[1].first), range_text(a, args[1]),
                  v->rank ? v->element : (keel_slice_char){0}, (keel_slice_char){0});
            return;
        }
        const Local *object = arg_local(c, args[0]);
        Text array_form = { .ok = true };
        put(&array_form, written.ptr, written.len);
        put_str(&array_form, "/2 \xe2\x86\x92 ");
        put_module_prefix(&array_form, home);
        put_str(&array_form, "_from_array");
        if (object && object->pointers == 0) put_str(&array_form, " &1");
        put_str(&array_form, " dim:2");
        emit(c, K_ISLAND_CALL, callee, &array_form);
        return;
    }

    /* `slice.of(v ...)` and `buffer.of(v)` over an `array` lower to the view's
       `from`, with dimension 0 from the table (backend §5.2) */
    if (gen && first && first->rank > 0 && k_token_spelled(name, "of") &&
        (module_named(home, "keel.slice") || module_named(home, "keel.buffer"))) {
        char elem[K_DETAIL_MAX];
        size_t n = k_spec_symbol(first->element, a->symbols, elem, sizeof elem);
        if (n == SIZE_MAX) return;
        if (first->rank > 1) {
            diag3(c, K_DIAG_FLAT_VIEW_OF_N_DIM_ARRAY, tok(a, callee), written, args[0].first < args[0].end ? tok(a, args[0].first) : written, first->dims);
            return;
        }
        bool slice = module_named(home, "keel.slice");
        if (!slice && argc == 1) {
            for (size_t i = 0; i + 5 <= first->element.len; i++)
                if (!memcmp(first->element.ptr + i, "const", 5) &&
                    (i == 0 || first->element.ptr[i - 1] == ' ') &&
                    (i + 5 == first->element.len || first->element.ptr[i + 5] == ' ')) {
                    diag3(c, K_DIAG_BUFFER_OVER_CONST, tok(a, callee), written, first->element, (keel_slice_char){0});
                    break;
                }
        }
        if (!slice && argc != 1) return;            /* buffer.of(p, n) is the ordinary verb */
        if (slice && argc > 3) return;
        put_module_prefix(&t, home);
        put_str(&t, "_");
        put(&t, elem, n);
        if (slice && argc > 1) { put_str(&t, "_of"); put_uint(&t, argc - 1); put_str(&t, " "); put_module_prefix(&t, home); put_str(&t, "_"); put(&t, elem, n); put_str(&t, "_from"); }
        else put_str(&t, slice ? "_from" : "_of");
        put_str(&t, " dim:1");
        emit(c, K_ISLAND_CALL, callee, &t);
        return;
    }

    if (gen) {
        char inst[K_DETAIL_MAX];
        size_t inst_len = SIZE_MAX;
        if (recv != SIZE_MAX || arr != SIZE_MAX) {
            size_t k = recv != SIZE_MAX ? recv : arr;
            if (k >= argc) return;
            Range r = args[k];
            if (punct(a, r.first, "&") && recv != SIZE_MAX) {
                diag3(c, K_DIAG_ADDRESS_IN_OBJECT_POSITION, tok(a, r.first), written, range_text(a, r), (keel_slice_char){0});
                return;
            }
            const Local *l = arg_local(c, r);
            bool wanted = recv != SIZE_MAX ? l && l->spec.kind == K_SPEC_MODIFIER && !l->rank
                                           : l && l->rank > 0;
            if (!wanted) {
                /* only what is certainly not a container is refused: a local seen
                   declared as something else, or a shape that is C. A name this pass
                   could not have seen declared is not refused. */
                if (l || container_shape(a, r) == SHAPE_C)
                    diag3(c, K_DIAG_NOT_A_CONTAINER_EXPRESSION, tok(a, r.first), written, range_text(a, r),
                          (keel_slice_char){0});
                return;
            }
            if (recv != SIZE_MAX) {
                const KSymbol *m = k_symbol_resolve(a->symbols, l->spec.modifier_name);
                if (!m || m->origin != module) {
                    if (m && m->origin && !sig.producer)
                        diag3(c, K_DIAG_WRONG_QUALIFIER, tok(a, callee), written, alias_of(a->symbols, m->origin), range_text(a, r));
                    return;
                }
                inst_len = k_spec_symbol(l->spec.text, a->symbols, inst, sizeof inst);
                /* get and set copy an element: an instance of a modifier is not copied by value */
                if ((k_token_spelled(name, "get") || k_token_spelled(name, "set")) && l->spec.arg_count) {
                    KLexer element;
                    TKPpKind pp;
                    KToken head = enter_text(l->spec.args[0], &element, &pp);
                    const KSymbol *e = head.len ? k_symbol_resolve(a->symbols, head) : NULL;
                    if (e && e->kind == K_SYM_MODIFIER)
                        diag3(c, K_DIAG_ELEMENT_COPY_IN_GET_SET, tok(a, callee), written, l->spec.args[0],
                              (keel_slice_char){0});
                }
            } else {
                Text head = { .ok = true };
                put_module_prefix(&head, home);
                char elem[K_DETAIL_MAX];
                size_t n = k_spec_symbol(l->element, a->symbols, elem, sizeof elem);
                if (n != SIZE_MAX && head.ok && head.n + 1 + n < sizeof inst) {
                    memcpy(inst, head.s, head.n);
                    inst[head.n] = '_';
                    memcpy(inst + head.n + 1, elem, n);
                    inst_len = head.n + 1 + n;
                }
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
            if (!spelled.ok) return;
            inst_len = k_spec_symbol((keel_slice_char){ spelled.n, spelled.s }, a->symbols, inst, sizeof inst);
        } else if (sig.found && c->has_target && c->target.kind == K_SPEC_MODIFIER) {
            const KSymbol *m = k_symbol_resolve(a->symbols, c->target.modifier_name);
            if (!m || m->origin != module) return;
            inst_len = k_spec_symbol(c->target.text, a->symbols, inst, sizeof inst);
        } else {
            /* from-without-target waits for the targets of an assignment and of
               a return: refusing now would refuse valid programs */
            return;
        }
        if (inst_len == SIZE_MAX) return;
        put(&t, inst, inst_len);
    } else {
        put_module_prefix(&t, home);
    }
    put_str(&t, "_");
    put(&t, declared.ptr, declared.len);
    if (sig.found && sig.overloaded && argc > 1) put_uint(&t, argc - 1);

    /* adaptation marks, from the declared parameter (spec §4.4, item 5) */
    for (size_t k = 0; sig.found && k < argc && k < sig.n; k++) {
        const Range *r = &args[k];
        if (sig.p[k].is_type) {
            if (!binder(home, sig.p[k].base)) { put_str(&t, " type:"); put_uint(&t, k + 1); }
            continue;
        }
        const Local *l = arg_local(c, *r);
        if (sig.p[k].is_array) {
            if (l && l->rank > 0) { put_str(&t, " dim:"); put_uint(&t, k + 1); }
            continue;
        }
        if (sig.p[k].pointer && l && l->pointers == 0) { put_str(&t, " &"); put_uint(&t, k + 1); }
    }
    emit(c, from_stack ? K_ISLAND_FROM_STACK : K_ISLAND_CALL, callee, &t);
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
    if (find_local(c, name)) return;
    Text t = { .ok = true };
    put(&t, name.ptr, name.len);
    put_str(&t, " \xe2\x86\x92 ");
    const KSymbol *s = k_symbol_resolve(a->symbols, name);
    if (s && (s->kind == K_SYM_CONSTANT || s->kind == K_SYM_VARIABLE) && s->origin) {
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

/* The module whose verbs a container of this local's type answers to. */
static const KAst *container_home(const Ctx *c, const Local *l) {
    if (l->spec.kind != K_SPEC_MODIFIER || l->rank) return NULL;
    const KSymbol *m = k_symbol_resolve(c->ast->symbols, l->spec.modifier_name);
    return m && m->origin ? m->origin->ast : NULL;
}

/* `<instance>_<verb><suffix>` for the container `l` and a call of `arity`
   arguments, object included; `adapt` says the object takes an `&`. */
static bool container_verb(const Ctx *c, const Local *l, const KAst *home, keel_slice_char verb,
                           size_t arity, Text *t, bool *adapt) {
    Sig sig = find_verb(home, verb, arity);
    if (!sig.found) return false;
    char inst[K_DETAIL_MAX];
    size_t n = k_spec_symbol(l->spec.text, c->ast->symbols, inst, sizeof inst);
    if (n == SIZE_MAX) return false;
    put(t, inst, n);
    put_str(t, "_");
    put(t, verb.ptr, verb.len);
    if (sig.overloaded && arity > 1) put_uint(t, arity - 1);
    if (sig.n && sig.p[0].pointer && l->pointers == 0) *adapt = true;
    return true;
}

/* The index expressions of the bracket groups that follow `x` from `open`, one
   after the other: `v[1,2][3]` gives 1, 2, 3. Returns how many, all counted. */
static size_t indices_of(const KAst *a, size_t open, Range out[8], size_t *last_close) {
    size_t count = 0;
    for (size_t o = open; punct(a, o, "["); ) {
        size_t close = close_bracket(a, o, a->tokens.len);
        if (close == SIZE_MAX) break;
        size_t start = o + 1, depth = 0;
        for (size_t j = o + 1; j <= close; j++) {
            if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
            else if (j < close && (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}"))) depth--;
            else if (j == close || (depth == 0 && punct(a, j, ","))) {
                if (count < 8) out[count] = (Range){ start, j };
                count++;
                start = j + 1;
            }
        }
        *last_close = close;
        o = close + 1;
    }
    return count;
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

/* `x[` at token `i`: every index or range-index over a symbol registered as an
   `array` or as an instance of a modifier is an island, even when the emission
   is the written text (spec §2.3). True when one was printed. */
static bool index_forms(Ctx *c, size_t i) {
    KAst *a = c->ast;
    size_t open = i + 1, close = close_bracket(a, open, a->tokens.len);
    if (close == SIZE_MAX) return false;
    KToken name = tok(a, i);
    const Local *l = find_local(c, name);
    if (!l || l->decl_at == i) return false;        /* not ours, or the declarator's own brackets */

    size_t commas = 0, range = SIZE_MAX, depth = 0;
    for (size_t j = open + 1; j < close; j++) {
        if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) depth++;
        else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) depth--;
        else if (depth == 0 && punct(a, j, ",")) commas++;
        else if (depth == 0 && punct(a, j, "..") && range == SIZE_MAX) range = j;
    }
    Text t = { .ok = true };

    /* an open end reads the container twice (spec §4.5, item 9): the path before
       it may not hold an index — `x[i][a..]` — for the read would repeat it */
    for (size_t o = close + 1; range == SIZE_MAX && punct(a, o, "["); ) {
        size_t next = close_bracket(a, o, a->tokens.len);
        if (next == SIZE_MAX) break;
        for (size_t j = o + 1, d = 0; j < next; j++) {
            if (punct(a, j, "(") || punct(a, j, "[") || punct(a, j, "{")) d++;
            else if (punct(a, j, ")") || punct(a, j, "]") || punct(a, j, "}")) d--;
            else if (d == 0 && punct(a, j, "..") && j > o + 1 && j + 1 == next) {
                diag3(c, K_DIAG_OPEN_RANGE_INDEX_ON_COMPLEX_PATH, name,
                      (keel_slice_char){ (size_t)(tok(a, next).ptr - tok(a, o).ptr) + 1, tok(a, o).ptr },
                      (keel_slice_char){0}, (keel_slice_char){0});
            }
        }
        o = next + 1;
    }

    /* an `array` takes its indices as they come, in one bracket or several */
    if (l->rank > 0) {
        if (range != SIZE_MAX) { check_inverted(c, name, open, range, close); return false; }   /* an array's view has no verb to name */
        Range idx[8];
        size_t last = close, indices = indices_of(a, open, idx, &last);
        put(&t, name.ptr, name.len);
        put_str(&t, " rank ");
        put_uint(&t, indices);
        if (indices < (size_t)l->rank)
            diag3(c, K_DIAG_PARTIAL_ARRAY_INDEX, name, l->dims, (keel_slice_char){0}, (keel_slice_char){0});
        long dims[8];
        size_t known = dims_of(c, l->dims, dims);
        for (size_t k = 0; k < indices && k < known && k < 8; k++) {
            long v;
            if (dims[k] >= 0 && decimal_of(c, idx[k], &v) && v >= dims[k])
                diag3(c, K_DIAG_ARRAY_INDEX_ABOVE_DIMENSION, tok(a, idx[k].first), range_text(a, idx[k]), l->dims,
                      (keel_slice_char){0});
        }
        return emit(c, K_ISLAND_ARRAY_INDEX, i, &t);
    }

    const KAst *home = container_home(c, l);
    if (!home || close == open + 1) return false;
    if (module_node(home)->dim_first != module_node(home)->dim_end) return false;   /* full-rank accessor: §5.3.1 */
    keel_slice_char text = { (size_t)(tok(a, close).ptr - tok(a, open).ptr) + 1, tok(a, open).ptr };
    bool adapt = false;
    put(&t, name.ptr, name.len);
    if (range == SIZE_MAX) {
        /* x[i, j] is *ptr(x, i, j) (spec §4.5, item 1) */
        put_str(&t, " \xe2\x86\x92 ");
        if (!container_verb(c, l, home, k_diag_text("ptr"), commas + 2, &t, &adapt)) {
            diag3(c, K_DIAG_NO_PTR_FOR_ARITY, name, l->spec.text, text, (keel_slice_char){0});
            return false;
        }
    } else {
        /* x[a..b] goes through the verb of the memory side of the pair; the
           base's two are named by the spec (§4.5, item 5) */
        if (commas) return false;
        check_inverted(c, name, open, range, close);
        const KAstNode *hm = module_node(home);
        bool base = hm->name_end > hm->name_first + 1 && k_token_spelled(tok(home, hm->name_first), "keel");
        keel_slice_char verb = module_named(home, "keel.buffer") ? k_diag_text("as_slice")
                             : module_named(home, "keel.slice") ? k_diag_text("of")
                             : (keel_slice_char){0};
        if (!verb.len) {
            /* a base type outside the pair has none; how a program's module declares
               its own is not written down (spec §4.5, item 7), so it is not judged */
            if (base) diag3(c, K_DIAG_NO_RANGE_INDEX_VERB, name, l->spec.text, text, (keel_slice_char){0});
            return false;
        }
        bool low = range > open + 1, high = range + 1 < close;
        put_str(&t, " ");
        put_source(&t, tok(a, open).ptr + 1, tok(a, close).ptr);
        put_str(&t, " \xe2\x86\x92 ");
        if (!container_verb(c, l, home, verb, low || high ? 3 : 1, &t, &adapt)) {
            diag3(c, K_DIAG_NO_RANGE_INDEX_VERB, name, l->spec.text, text, (keel_slice_char){0});
            return false;
        }
        if (low && !high) {                         /* the limit is length(x) */
            bool ignored = false;
            put_str(&t, " ");
            if (!container_verb(c, l, home, k_diag_text("length"), 1, &t, &ignored)) return false;
        }
    }
    if (adapt) put_str(&t, " &1");
    return emit(c, range == SIZE_MAX ? K_ISLAND_INDEX : K_ISLAND_RANGE_INDEX, i, &t);
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

        /* `alias.verb(` and `verb(` */
        if (!find_local(c, t) && punct(a, i + 1, ".") && ident(a, i + 2) && punct(a, i + 3, "(")) {
            const KModule *module = module_alias(a->symbols, t);
            if (module) { call(c, i, i + 2, i + 3, module); continue; }
        }
        if (!find_local(c, t) && punct(a, i + 1, ".") && ident(a, i + 2) && !punct(a, i + 3, "(")) {
            const KModule *module = module_alias(a->symbols, t);
            if (module) { qualified_name(c, i, module); i += 2; continue; }
        }
        if (punct(a, i + 1, "(")) {
            if (!find_local(c, t)) {
                bool own = false;
                for (size_t k = 0; k < a->nodes.len && !own; k++)
                    own = node_at(a, k)->kind == K_AST_FUNCTION && k_symtab_same_name(tok(a, node_at(a, k)->name_first), t);
                if (own) call(c, i, i, i + 1, NULL);
            }
            continue;
        }
        /* a label is not a use */
        if (punct(a, i + 1, ":") && (i == n->first || punct(a, i - 1, "{") || punct(a, i - 1, ";") ||
                                     punct(a, i - 1, "}") || punct(a, i - 1, ":")))
            continue;
        if (punct(a, i + 1, "[") && index_forms(c, i)) continue;
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
            if (ctx.locals[k].depth == 0) ctx.globals[ctx.global_count++] = ctx.locals[k];
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
