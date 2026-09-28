/* Passage 1's second half (parser-design §3, [P3]): the file's imports,
   resolved into the symbol table before any declaration is collected.
   It has to be before, and not after, because keel's grammar depends on
   the table — `buffer i32 x;` is a keel declaration only once `buffer`
   is known as a modifier of arity 1 (parser-design §1). */
#include "engine/ast.h"
#include "engine/loader.h"
#include "engine/parser.h"

/* The name as written, from the first token of the path to the last:
   `keel.slice` is one slice of the source, dots included, and no
   allocation is needed for it. */
static keel_slice_char spelled(const KAst *a, size_t first, size_t end) {
    if (first >= end) return (keel_slice_char){ 0, NULL };
    KToken lead = a->tokens[first].token;
    KToken last = a->tokens[end - 1].token;
    return (keel_slice_char){ (size_t)((last.ptr + last.len) - lead.ptr), lead.ptr };
}

/* `types` injects the module's type and modifier names without a
   qualifier, and nothing else — keel-spec §4.1 item 6: "Não injeta
   funções, variáveis nem constantes de enum." Arity travels with a
   modifier, because that is what decides how many arguments its uses
   read. */
static bool inject_types(KSymbolTable *symtab, const KModule *module) {
    for (size_t i = 0; i < module->symbol_count; i++) {
        const KSymbol *sym = &module->symbols[i];
        if (sym->kind != K_SYM_TYPE && sym->kind != K_SYM_MODIFIER) continue;
        if (!k_symtab_insert(symtab, sym->name, sym->kind, sym->arity)) return false;
        symtab->items[symtab->count - 1] = *sym;
    }
    return true;
}

/* One import. `at` is what a diagnostic points at. */
static bool resolve_one(KLoader *loader, KSymbolTable *symtab, KDiagnosticSink *diag,
                        keel_slice_char module_name, bool has_types,
                        keel_slice_char alias, keel_slice_char at) {
    KModule *module = NULL;
    KLoadResult result = loader->load(loader->tool, module_name, &module);

    if (result == K_LOAD_CYCLE) {
        /* the chain comes from the tool's load stack, the position from
           here — neither half has both (cgen-tool §3.1) */
        KDiagArgs args = { { module_name } };
        k_diag_emit(diag, K_DIAG_CIRCULAR_IMPORT, at, args);
        return false;
    }
    if (result == K_LOAD_NOT_FOUND) {
        k_diag_emit(diag, K_DIAG_MODULE_NOT_FOUND, at, (KDiagArgs){{module_name}});
        return false;
    }
    if (result != K_LOAD_OK && result != K_LOAD_ALREADY) return false;
    if (module == NULL) return false;

    if (has_types && !inject_types(symtab, module)) return false;
    /* Keep the real qualifier as well as any alias; neither changes origin. */
    if (!k_symtab_insert(symtab, module_name, K_SYM_MODULE, 0)) return false;
    symtab->items[symtab->count - 1].origin = module;
    if (alias.len != 0) {
        if (!k_symtab_insert(symtab, alias, K_SYM_MODULE, 0)) return false;
        symtab->items[symtab->count - 1].origin = module;
    }
    return true;
}

bool k_resolve_imports(const KAst *ast, KLoader *loader, KSymbolTable *symtab,
                        KDiagnosticSink *diag) {
    bool ok = true;

    /* keel-spec §4.1: every file has the implicit `import keel types;`,
       which is where `i32` and the other primitive names come from. The
       prelude itself is the one file that does not get it. */
    const KAstNode *mod = &ast->nodes[ast->module];
    keel_slice_char self = spelled(ast, mod->name_first, mod->name_end);
    static const char keel_name[] = "keel";
    keel_slice_char prelude = { 4, (char *)keel_name };
    if (!k_symtab_same_name(self, prelude)) {
        if (!resolve_one(loader, symtab, diag, prelude, true,
                         (keel_slice_char){ 0, NULL }, self))
            ok = false;
    }

    for (size_t i = 0; i < ast->node_count; i++) {
        const KAstNode *n = &ast->nodes[i];
        if (n->kind != K_AST_IMPORT) continue;
        keel_slice_char name = spelled(ast, n->name_first, n->name_end);
        keel_slice_char alias = n->alias != (size_t)-1
            ? ast->tokens[n->alias].token
            : (keel_slice_char){ 0, NULL };
        if (!resolve_one(loader, symtab, diag, name, n->has_types, alias,
                         ast->tokens[n->anchor].token))
            ok = false;
    }
    return ok;
}
