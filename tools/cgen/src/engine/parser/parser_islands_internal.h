#ifndef KEEL_ENGINE_PARSER_ISLANDS_INTERNAL_H
#define KEEL_ENGINE_PARSER_ISLANDS_INTERNAL_H

/* Internal to pass 3 (parser_islands*.c): the types, macros and prototypes shared by its
 * six files. Not the API of the engine: nothing outside parser_islands*.c includes it. */
#include <stdint.h>
#include <string.h>
#include "engine/islands.h"
#include "engine/instances.h"
#include "engine/names.h"
#include "engine/parser/parser.h"

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
    /* provenance (spec §4.12): the symbols it depends on, by index among the
       locals, and its two states */
    int deps[8];
    int ndeps;
    bool local_origin;          /* depends on an automatic `array` of the function */
    bool invalid;
    bool told;                  /* its use after invalidation was reported */
    keel_slice_char invalid_by; /* the verb that invalidated it */
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
    struct {                    /* the `later` defers of the blocks open, for defer-later-shadowed */
        size_t at, body_first, body_end;
        int depth;
        keel_slice_char shadow;
        int shadow_depth;
        bool told;
    } later[16];
    size_t later_count;
    const KAstNode *fn;         /* the declaration being walked */
    struct { keel_slice_char name; int parent; size_t until; } pending[8];   /* products of a construction */
    size_t pending_count;
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

typedef struct {
    Param p[K_PARAMS_MAX];
    size_t n;
    bool found;
    bool overloaded;            /* declared in more than one arity (backend §2.1.1) */
    int rank_binder;            /* the vector accessor `size_t idx[static N]`: N's place among the
                                   `dim` binders, which names the rank suffix `_<N>` (§2.1.1, rule 6); -1 */
    bool producer;              /* returns the module's own type: `of`, `from`, `clone` (spec §4.4, item 1) */
    const KAstNode *fn;         /* the declaration found */
} Sig;
typedef enum { SHAPE_NAME, SHAPE_CONTAINER, SHAPE_C } Shape;
#define K_CLOSURE_MAX 64

/* prototypes of the functions that more than one file of the pass calls */
KToken isl_tok(const KAst *a, size_t i);
bool isl_live(const KAst *a, size_t i);
bool isl_punct(const KAst *a, size_t i, const char *s);
bool isl_word(const KAst *a, size_t i, const char *s);
bool isl_ident(const KAst *a, size_t i);
size_t isl_index_at(const KAst *a, size_t from, const char *ptr);
KToken isl_enter(const KAst *a, size_t i, KLexer *lexer, TKPpKind *pp);
size_t isl_close_paren(const KAst *a, size_t open, size_t end);
size_t isl_close_bracket(const KAst *a, size_t open, size_t end);
void isl_put(Text *t, const char *p, size_t n);
void isl_put_str(Text *t, const char *s);
void isl_put_uint(Text *t, size_t v);
void isl_put_tokens(Text *t, const KAst *a, size_t first, size_t end);
void isl_put_glued(Text *t, const KAst *a, size_t first, size_t end);
void isl_put_source(Text *t, const char *p, const char *end);
keel_slice_char isl_keep(Ctx *c, const Text *t);
bool isl_emit(Ctx *c, KIslandKind kind, size_t anchor, const Text *t);
const Local *isl_find_local(const Ctx *c, keel_slice_char name);
bool isl_digits(keel_slice_char s, long *out);
bool isl_decimal_of_token(const Ctx *c, KToken t, long *out);
bool isl_decimal_of(const Ctx *c, Range r, long *out);
size_t isl_dims_of(const Ctx *c, keel_slice_char text, long out[8]);
void isl_add_local(Ctx *c, Local local);
int isl_rank_of(const char *p, const char *end);
const KAstNode *isl_node_at(const KAst *a, size_t i);
const KAstNode *isl_module_node(const KAst *a);
bool isl_generic(const KAst *a);
void isl_put_module_prefix(Text *t, const KAst *a);
bool isl_module_named(const KAst *a, const char *name);
bool isl_module_in_base(const KAst *a);
bool isl_binder(const KAst *a, keel_slice_char name);
keel_slice_char isl_modifier_of(const KAst *a);
size_t isl_params_of(const KAst *a, const KAstNode *fn, Param *out, size_t cap);
Sig isl_find_verb(const KAst *a, keel_slice_char verb, size_t arity);
/* The suffix of verb `sig` on the container type k (backend §2.1.1): `_<N>` for
   the vector accessor, with N the value of the instance's `dim`; the arity for an
   overloaded verb of `arity` arguments; nothing otherwise. False when the rank is
   not known. */
bool isl_put_verb_suffix(const Ctx *c, const KType *k, const Sig *sig, size_t arity, Text *t);
const KModule *isl_module_alias(const KSymbolTable *symbols, keel_slice_char name);
const KModule *isl_module_by_name(const KSymbolTable *symbols, const char *name);
bool isl_protocol_decl(const Ctx *c, const KAst *home, keel_slice_char name, KProtocolDecl *out, int depth);
bool isl_satisfies(Ctx *c, const KAst *own, keel_slice_char receiver, const KProtocolDecl *pd, Text *missing, KToken at);
bool isl_returns_associated(const KAst *home, const KAstNode *fn);
const KProtocolVerb *isl_verb_for_return(const KAst *home, const KAstNode *fn, const KProtocolDecl *pd);
keel_slice_char isl_range_text(const KAst *a, Range r);
void isl_diag3(Ctx *c, KDiagId id, keel_slice_char at, keel_slice_char x, keel_slice_char y, keel_slice_char z);
keel_slice_char isl_alias_of(const KSymbolTable *symbols, const KModule *m);
KToken isl_enter_text(keel_slice_char text, KLexer *lexer, TKPpKind *pp);
Shape isl_container_shape(const KAst *a, Range r);
const Local *isl_arg_local(const Ctx *c, Range r);
KType *isl_kt(Ctx *c, int t);
const char *isl_kt_symbol(const Ctx *c, int t);
int isl_kt_new(Ctx *c);
void isl_kt_set_symbol(KType *k, const char *s, size_t n);
const KAst *isl_ast_of(const Ctx *c, const KModule *m);
int isl_kt_from_text(Ctx *c, const KSymbolTable *symbols, keel_slice_char text);
size_t isl_binders_of(const KAst *home, keel_slice_char out[8]);
int isl_kt_from_home(Ctx *c, const KAst *home, keel_slice_char text, int inst);
int isl_ret_type(Ctx *c, const KAst *home, const KAstNode *fn, int inst);
int isl_type_of(Ctx *c, Range r);
void isl_resolution_begin(Ctx *c);
void isl_resolution_end(Ctx *c);
bool isl_call_at(Ctx *c, size_t i);
bool isl_tag_of_some_set(const KAst *where, keel_slice_char name);
void isl_name_island(Ctx *c, size_t i);
void isl_qualified_name(Ctx *c, size_t i, const KModule *module);
bool isl_is_arena(const KAst *a, const KSpecifier *spec);
void isl_note_shadow(Ctx *c, KToken name);
bool isl_binder_of(const KAst *a, const KAstNode *fn, KToken name);
bool isl_c_declares(const KAst *a, size_t i, size_t from);
size_t isl_declaration(Ctx *c, size_t i, const KSpecifier *spec, KLexer *lexer, KToken next, bool signature,
                   bool declarators);
size_t isl_array_decl(Ctx *c, size_t i, bool signature);
bool isl_container_verb(const KType *k, const KAst *home, keel_slice_char verb, size_t arity, Text *tx, bool *adapt);
bool isl_path_forms(Ctx *c, size_t i);
bool isl_signature_of(const KAstNode *n, size_t i);
void isl_block_constexpr(Ctx *c, size_t i);
bool isl_assignment_op(const KAst *a, size_t i);
bool isl_after_operand(const KAst *a, size_t i);
void isl_check_uses(Ctx *c, size_t i);
void isl_put_range(Text *t, const KAst *a, size_t first, size_t end);
size_t isl_enclosing_brace(const KAst *a, size_t from, size_t i);
const char *isl_control_before(const KAst *a, size_t from, size_t j);
bool isl_tags_decl(const KAst *where, keel_slice_char name, KTagsDecl *out);
void isl_extent_island(Ctx *c, const KAstNode *n);
size_t isl_column_access(Ctx *c, size_t i, size_t open, bool *any);
void isl_walk(Ctx *c, const KAstNode *n);
Range isl_param_type(const KAst *home, const Param *p);
size_t isl_call_args(const KAst *a, size_t open, size_t close, Range *first);
bool isl_verb_available(Ctx *c, const KAst *home, const KAstNode *fn, int t, const char **cause);
size_t isl_instances_in(Ctx *c, int t, int *out, size_t n, size_t cap, int depth);
int isl_kt_deep(Ctx *c, const KType *nodes, int t);
int isl_local_of(const Ctx *c, Range r);
void isl_produce(Ctx *c, int p, const int *parents, int np);
void isl_roles_at_call(Ctx *c, const KAst *home, const Sig *sig, const Range *args, size_t argc, size_t callee,
                   keel_slice_char verb);
void isl_record_seed(Ctx *c, int t, const char *symbol, KClosureKind kind, const KModule *module);
void isl_instance_decl(Ctx *c, const KAstNode *n);
void isl_check_generic(Ctx *c);
void isl_check_modifiers(Ctx *c);
void isl_check_collisions(Ctx *c);
void isl_check_alias_types(Ctx *c);
void isl_check_name(Ctx *c, KToken name, bool top, bool function);
bool isl_spec_byref(const KAst *a, const KSpecifier *spec);
void isl_check_arguments(Ctx *c, const KSpecifier *spec, KToken at);
void isl_check_declarations(Ctx *c);
void isl_check_constexpr(Ctx *c, size_t kw, size_t end);
void isl_check_protocols(Ctx *c);
void isl_check_jumps(Ctx *c, const KAstNode *fn);
void isl_check_layout(Ctx *c);

#endif
