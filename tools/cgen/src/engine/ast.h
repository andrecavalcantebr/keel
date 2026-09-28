/* A source-backed syntax tree for the file-level part of keel.
 * Token indexes are half-open. Source and token storage outlive the tree. */
#ifndef CGEN_ENGINE_AST_H
#define CGEN_ENGINE_AST_H

#include "engine/lexer.h"
#include "engine/loader.h"
#include "keel/keel_buffer_KLexeme.h"
#include "keel/keel_buffer_KAstNode.h"
#include "keel/keel_buffer_KInstanceUse.h"

typedef struct KAst {
    keel_slice_char source;
    keel_buffer_KLexeme tokens;
    keel_buffer_KAstNode nodes;
    size_t module;              /* index of K_AST_MODULE */
    KSymbolTable *symbols; /* caller-owned, retained collection environment */
    size_t error_token;
    keel_buffer_KInstanceUse instances;
} KAst;

/* Both passes return the required count. A NULL/zero output only counts. */
size_t k_lexemes(keel_slice_char source, KLexeme *out, size_t cap,
                 KDiagnosticSink *diagnostics);
/* Caller supplies nodes via ast->nodes; each parse resets its length.
 * Returns false on malformed/truncated input or insufficient capacity. */
bool k_parse_ast(KAst *ast);
bool k_parse_headers(KAst *ast);
bool k_collect_ast(KAst *ast, KSymbolTable *symbols);
/* Passage 1 (parser-design §3): resolves the file's imports — the
   implicit `import keel types;` first — into `symtab`, by calling back
   through `loader`. Emits `circular-import` at the offending `import`;
   emits module-not-found at the import when no root contains the module.
   Returns false if any import did not resolve. */
bool k_resolve_imports(const KAst *ast, KLoader *loader,
                       KSymbolTable *symtab, KDiagnosticSink *diag);
/* Writes at most output.len bytes, returns the required length. */
size_t k_dump_ast(const KAst *ast, const char *path, keel_slice_char output);

#endif
