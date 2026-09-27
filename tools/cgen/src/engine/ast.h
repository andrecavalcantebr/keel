/* A source-backed syntax tree for the file-level part of keel.
 * Token indexes are half-open. Source and token storage outlive the tree. */
#ifndef CGEN_ENGINE_AST_H
#define CGEN_ENGINE_AST_H

#include "engine/lexer.h"
#include "engine/loader.h"

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

typedef struct {
    keel_slice_char source;
    KLexeme *tokens;
    size_t token_count;
    KAstNode *nodes;
    size_t node_count;
    size_t module;              /* index of K_AST_MODULE */
} KAst;

/* Both passes return the required count. A NULL/zero output only counts. */
size_t k_lexemes(keel_slice_char source, KLexeme *out, size_t cap,
                 KDiagnosticSink *diagnostics);
/* Returns false on malformed/truncated input or insufficient node storage. */
bool k_parse_ast(KAst *ast, KAstNode *nodes, size_t cap);
/* Passage 1 (parser-design §3): resolves the file's imports — the
   implicit `import keel types;` first — into `symtab`, by calling back
   through `loader`. Emits `circular-import` at the offending `import`;
   a module the loader could not find is left for the tool to report,
   which is the half that knows the roots. Returns false if any import
   did not resolve. */
bool k_resolve_imports(const KAst *ast, KLoader *loader,
                       KSymbolTable *symtab, KDiagnosticSink *diag);
/* Writes at most output.len bytes, returns the required length. */
size_t k_dump_ast(const KAst *ast, const char *path, keel_slice_char output);

#endif
