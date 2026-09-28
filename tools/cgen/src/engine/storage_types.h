#ifndef CGEN_STORAGE_TYPES_H
#define CGEN_STORAGE_TYPES_H
#include "engine/lexer.h"
typedef enum {
    K_SYM_TYPE,        /* typedef, struct/union/enum nomeado, tipo de módulo */
    K_SYM_MODIFIER,    /* aridade e espécie de cada parâmetro                */
    K_SYM_FUNCTION,    /* retorno declarado e aridades                       */
    K_SYM_VARIABLE,
    K_SYM_CONSTANT,    /* constexpr, constante de enum                       */
    K_SYM_TAGS,        /* conjunto fechado e seus valores                    */
    K_SYM_MODULE,      /* alias ativo de import                              */
    K_SYM_EXTERN_C     /* nome de extern_c: só a distinção variável/função   */
} KSymKind;

struct KModule;

typedef struct {
    keel_slice_char name;   /* the keel name, as spelled */
    KSymKind kind;
    int arity;   /* K_SYM_MODIFIER: the declaring module's own dim+tags+type
                    binder count (parser-design §4.3: "a assinatura do
                    módulo fixa a quantidade... de todos os seus
                    modificadores" — every modifier of a module shares its
                    arity). K_SYM_FUNCTION: parameter count. Unused (0) for
                    the other kinds so far — this grows with later etapas,
                    same way KModuleHeader grew its binders. */
    const struct KModule *origin; /* declaration identity, also alias target */
    int dim_arity;
    keel_slice_char value; /* explicit constexpr initializer, if present */
} KSymbol;

typedef struct {
    KToken token;
    TKPpKind pp_kind;
    bool directive;
} KLexeme;

typedef enum {
    K_AST_MODULE, K_AST_IMPORT, K_AST_IMPORT_C, K_AST_EXTERN_C,
    K_AST_MODIFIER, K_AST_TAGS, K_AST_TYPE, K_AST_CONSTEXPR,
    K_AST_FUNCTION, K_AST_VARIABLE, K_AST_OPAQUE
} KAstKind;

typedef struct {
    KAstKind kind;
    size_t first, end;          /* token span [first, end) */
    size_t anchor;              /* token used for diagnostics and the dump */
    size_t name_first, name_end;/* name/path span [first, end); empty if absent */
    size_t body_first, body_end;/* function/aggregate body, if any */
    size_t dim_first, dim_end;  /* module binders; empty ranges if absent */
    size_t tags_first, tags_end;
    size_t type_first, type_end;
    size_t alias;               /* import alias token; SIZE_MAX if absent */
    bool has_types;             /* import ... types */
    bool is_public;             /* default visibility is public */
    bool is_inline;
} KAstNode;

#define K_INSTANCE_NAME_CAP 256 /* 255 characters plus terminating zero */
/* Direct, concrete uses only; transitive generic instantiation is a later pass. */
typedef struct KInstanceUse {
    size_t first, end;
    char symbol[K_INSTANCE_NAME_CAP];
    size_t symbol_len;
} KInstanceUse;

#endif
