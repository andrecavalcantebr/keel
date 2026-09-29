/* Pass 3 (parser-design §3), stage 4a: type, name, call, from-stack, ref and
 * implicit-init islands, read from the token stream of each function and
 * variable declaration — signature and body alike.
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
} Local;

typedef struct {
    KAst *ast;
    KDiagnosticSink *diag;
    Local locals[K_LOCALS_MAX];
    size_t local_count;
    int depth;
    bool failed;
    bool has_target;            /* an initializer is being read... */
    KSpecifier target;          /* ...of an object declared with this type */
} Ctx;

typedef struct { char s[K_DETAIL_MAX]; size_t n; bool ok; } Text;

typedef struct {
    bool is_type;               /* `type X` */
    bool pointer;               /* declared as pointer (or array) */
    keel_slice_char base;       /* first type name of the parameter */
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

/* islands stay sorted by anchor: a later declarator's island can be found
   before an earlier declarator's initializer is read */
static bool emit(Ctx *c, KIslandKind kind, size_t anchor, const Text *t) {
    KAst *a = c->ast;
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

static void add_local(Ctx *c, keel_slice_char name, const KSpecifier *spec, int pointers) {
    if (c->local_count == K_LOCALS_MAX) {
        if (!c->failed) {
            k_diag_emit(c->diag, K_DIAG_CAPACITY, name, (KDiagArgs){{name}});
            c->failed = true;
        }
        return;
    }
    c->locals[c->local_count++] = (Local){ name, *spec, pointers, c->depth };
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
            Param p = { .is_type = word(a, start, "type") };
            for (size_t k = start; k < j; k++) {
                if (punct(a, k, "*") || punct(a, k, "[")) p.pointer = true;
                if (!p.base.len && ident(a, k) && !(p.is_type && k == start)) p.base = tok(a, k);
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

typedef struct { size_t first, end; } Range;

/* One call: `callee` is the first token of the written callee, `verb` its
   last, `open` the '(' — `module` is the qualifier's module, NULL for a call
   to the file's own function. Prints nothing when the call is not resolved. */
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
    bool from_stack = module && module_named(home, "keel.arena") && k_token_spelled(name, "from_stack");
    Sig sig = from_stack ? find_verb(home, k_diag_text("from_array"), SIZE_MAX)
                         : find_verb(home, name, argc);
    if (!sig.found) return;
    keel_slice_char declared = from_stack ? k_diag_text("from_array") : name;

    Text t = { .ok = true };
    put_glued(&t, a, callee, verb + 1);
    put_str(&t, "/");
    put_uint(&t, argc);
    put_str(&t, " \xe2\x86\x92 ");

    if (module && generic(home)) {
        /* The instance comes from the object argument, or, for a producer that
           takes none, from the declared type of the object being initialized
           (spec §4.4, item 4). */
        keel_slice_char modifier = modifier_of(home), text = {0};
        size_t receiver = SIZE_MAX;
        bool selection = false;
        for (size_t k = 0; k < sig.n; k++) {
            if (sig.p[k].is_type) selection = selection || binder(home, sig.p[k].base);
            else if (receiver == SIZE_MAX && k_symtab_same_name(sig.p[k].base, modifier)) receiver = k;
        }
        Text written = { .ok = true };
        const KSpecifier *spec = NULL;
        if (receiver != SIZE_MAX) {
            if (receiver >= argc) return;
            const Range *r = &args[receiver];
            const Local *l = r->end == r->first + 1 && ident(a, r->first) ? find_local(c, tok(a, r->first)) : NULL;
            if (l) spec = &l->spec;
            if (!spec || spec->kind != K_SPEC_MODIFIER) return;
            text = spec->text;
        } else if (selection) {
            /* the written types are the instance's arguments, in parameter order */
            put(&written, modifier.ptr, modifier.len);
            for (size_t k = 0; k < sig.n && k < argc; k++) {
                if (!sig.p[k].is_type || !binder(home, sig.p[k].base)) continue;
                put_str(&written, " ");
                put_tokens(&written, a, args[k].first, args[k].end);
            }
            if (!written.ok) return;
            text = (keel_slice_char){ written.n, written.s };
        } else if (c->has_target && c->target.kind == K_SPEC_MODIFIER) {
            spec = &c->target;
            text = spec->text;
        } else {
            return;
        }
        if (spec) {
            const KSymbol *m = k_symbol_resolve(a->symbols, spec->modifier_name);
            if (!m || m->origin != module) return;
        }
        char inst[K_DETAIL_MAX];
        size_t n = k_spec_symbol(text, a->symbols, inst, sizeof inst);
        if (n == SIZE_MAX) return;
        put(&t, inst, n);
    } else {
        put_module_prefix(&t, home);
    }
    put_str(&t, "_");
    put(&t, declared.ptr, declared.len);
    if (sig.overloaded && argc > 1) put_uint(&t, argc - 1);

    /* adaptation marks, from the declared parameter (spec §4.4, item 5) */
    for (size_t k = 0; k < argc && k < sig.n; k++) {
        const Range *r = &args[k];
        if (sig.p[k].is_type) {
            if (!binder(home, sig.p[k].base)) { put_str(&t, " type:"); put_uint(&t, k + 1); }
            continue;
        }
        if (!sig.p[k].pointer || r->end != r->first + 1 || !ident(a, r->first)) continue;
        const Local *l = find_local(c, tok(a, r->first));
        if (l && l->pointers == 0) { put_str(&t, " &"); put_uint(&t, k + 1); }
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
                          bool signature) {
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

    bool external = i > 0 && word(a, i - 1, "extern");
    TKPpKind pp;
    for (KToken d = next; d.len; ) {
        KDeclarator decl;
        KToken after;
        if (!(k_token_is_punct(d, "*") || k_token_is_punct(d, "(") || k_token_is_ident(d))) break;
        if (!k_scan_declarator(lexer, d, &decl, &after, &pp) || !decl.name.len) break;
        if (decl.has_function_suffix && !decl.parenthesized) break;   /* a prototype */
        size_t at = index_at(a, i, decl.name.ptr);
        add_local(c, decl.name, spec, decl.pointer_depth + (decl.has_array_suffix ? 1 : 0));

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

static void walk(Ctx *c, const KAstNode *n) {
    KAst *a = c->ast;
    c->local_count = 0;
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
        if (i == n->name_first || !ident(a, i)) continue;
        if (i > n->first && (punct(a, i - 1, ".") || punct(a, i - 1, "->"))) continue;

        KToken t = tok(a, i);
        if (k_token_spelled(t, "ref") && ident(a, i + 1) &&
            (punct(a, i - 1, "*") || word(a, i - 1, "const") || word(a, i - 1, "volatile"))) {
            Text d = { .ok = true };
            put(&d, tok(a, i + 1).ptr, tok(a, i + 1).len);
            emit(c, K_ISLAND_REF, i, &d);
            continue;
        }

        /* a keel type, qualified or not */
        KLexer lexer;
        TKPpKind pp;
        KSpecifier spec;
        KToken next = {0};
        KToken first = enter(a, i, &lexer, &pp);
        if (k_scan_known_type(&lexer, first, a->symbols, &spec, &next, &pp) && spec.kind != K_SPEC_NONE) {
            bool signature = n->body_first != SIZE_MAX && i < n->body_first;
            if (n->body_first == SIZE_MAX && n->kind == K_AST_FUNCTION) signature = true;
            i = declaration(c, i, &spec, &lexer, next, signature);
            continue;
        }

        /* `alias.verb(` and `verb(` */
        if (!find_local(c, t) && punct(a, i + 1, ".") && ident(a, i + 2) && punct(a, i + 3, "(")) {
            const KModule *module = module_alias(a->symbols, t);
            if (module) { call(c, i, i + 2, i + 3, module); continue; }
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
        name_island(c, i);
    }
}

bool k_collect_islands(KAst *a, KDiagnosticSink *diag) {
    a->islands.len = 0;
    a->island_text.len = 0;
    if (!a->symbols || generic(a)) return true;
    Ctx ctx = { .ast = a, .diag = diag };
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
