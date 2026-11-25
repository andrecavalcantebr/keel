/* A source-backed syntax tree for the file-level part of keel.
 * Token indexes are half-open. Source and token storage outlive the tree. */
#ifndef CGEN_ENGINE_AST_H
#define CGEN_ENGINE_AST_H

#include "engine/lexer/lexer.h"
#include "engine/loader.h"
#include "keel/keel_buffer_KLexeme.h"
#include "keel/keel_buffer_KAstNode.h"
#include "keel/keel_buffer_KInstanceUse.h"
#include "keel/keel_buffer_KIsland.h"
#include "keel/keel_buffer_KClosure.h"
#include "keel/keel_buffer_KUnavailable.h"
#include "keel/keel_buffer_char.h"

typedef struct KAst {
    keel_slice_char source;
    keel_buffer_KLexeme tokens;
    keel_buffer_KAstNode nodes;
    size_t module;              /* index of K_AST_MODULE */
    KSymbolTable *symbols; /* caller-owned, retained collection environment */
    size_t error_token;
    bool error_is_capacity;    /* error_token's failure is a capacity, not a syntax error */
    bool error_is_extent_limit; /* error_token is an `extent` with more than K_EXTENT_MAX_DIMS groups */
    bool error_is_tags_limit;  /* error_token is a `tags` set with more than K_TAGS_MAX_ITEMS items */
    bool imports_or_declares_drafts;
    keel_buffer_KInstanceUse instances;
    keel_buffer_KIsland islands;   /* pass 3, sorted by anchor */
    keel_buffer_char island_text;  /* the islands' detail columns */
    keel_buffer_KClosure closure;      /* the closed set of instances (stage 4i) */
    keel_buffer_KUnavailable unavailable;  /* the verbs each of them loses */
    keel_buffer_char closure_text; /* the closure's strings: arguments, causes */
    size_t *decl_slots;        /* k_ast_index_decls: a hash of the declared names, NULL when not built (the emitter then scans the nodes) */
    size_t decl_slots_n;       /* its slots, a power of two */
} KAst;

/* Both passes return the required count. A NULL/zero output only counts. */
size_t k_lexemes(keel_slice_char source, KLexeme *out, size_t cap,
                 KDiagnosticSink *diagnostics);
/* Caller supplies nodes via ast->nodes; each parse resets its length.
 * Returns false on malformed/truncated input or insufficient capacity. */
bool k_parse_ast(KAst *ast);
bool k_parse_headers(KAst *ast);
bool k_collect_ast(KAst *ast, KSymbolTable *symbols);
/* The slots k_ast_index_decls needs for `ast` (a power of two, at least twice the nodes). */
size_t k_ast_decl_slots(const KAst *ast);
/* Indexes the declared names (types, tags, values) of the nodes in `slots`, k_ast_decl_slots(ast)
 * zeroed entries: k_em_declared_at then finds a name without scanning every node. */
void k_ast_index_decls(KAst *ast, size_t *slots, size_t n);
/* The first declaration, in node order, named like `name`; NULL when there is none. */
const KAstNode *k_ast_decl_named(const KAst *ast, const char *name, size_t len);
/* Passage 1 (parser-design §3): resolves the file's imports — the
   implicit `import keel types;` first — into `symtab`, by calling back
   through `loader`. Emits `circular-import` at the offending `import`;
   emits module-not-found at the import when no root contains the module.
   Returns false if any import did not resolve. */
bool k_resolve_imports(const KAst *ast, KLoader *loader,
                       KSymbolTable *symtab, KDiagnosticSink *diag);
/* keel-spec §2.4: `()`, `[]` and `{}` pair in every alternative of a
   conditional group, and the alternatives agree (unmatched-delimiter,
   delimiter-mismatch-across-branches). */
bool k_check_delimiters(const KAst *ast, KDiagnosticSink *diag);
/* Writes at most output.len bytes, returns the required length. */
size_t k_dump_ast(const KAst *ast, const char *path, keel_slice_char output);

#endif
