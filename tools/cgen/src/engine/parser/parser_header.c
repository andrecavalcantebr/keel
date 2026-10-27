#include "engine/ast.h"
#include "engine/loader.h"
#include "engine/parser/parser.h"
#include <string.h>

/* ---- k_scan_module_decl ---- */

bool k_scan_module_decl(KLexer *lexer, KModuleHeader *out,
                        KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken name_first = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_ident(name_first)) return false;
    out->module_name = k_scan_qualified_name(lexer, name_first, next_out, next_pp_kind_out);
    out->dim_count = out->tag_count = out->type_count = 0;

    if (k_token_is_ident_named(*next_out, "dim")) {
        KToken first = k_lexer_next(lexer, next_pp_kind_out);
        k_scan_ident_list(lexer, first, out->dims, 8, &out->dim_count, next_out, next_pp_kind_out);
    }
    if (k_token_is_ident_named(*next_out, "tags")) {
        KToken first = k_lexer_next(lexer, next_pp_kind_out);
        k_scan_ident_list(lexer, first, out->tags, 8, &out->tag_count, next_out, next_pp_kind_out);
    }
    if (k_token_is_ident_named(*next_out, "type")) {
        KToken first = k_lexer_next(lexer, next_pp_kind_out);
        k_scan_ident_list(lexer, first, out->types, 8, &out->type_count, next_out, next_pp_kind_out);
    }

    if (!k_token_is_punct(*next_out, ";")) return false;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return true;
}

bool k_scan_import(KLexer *lexer, KImportDecl *out,
                   KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken name_first = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_ident(name_first)) return false;
    out->module_name = k_scan_qualified_name(lexer, name_first, next_out, next_pp_kind_out);
    out->alias = (keel_slice_char){0};
    out->has_types = false;

    if (k_token_is_ident_named(*next_out, "as")) {
        out->alias = k_lexer_next(lexer, next_pp_kind_out);
        if (!k_token_is_ident(out->alias)) return false;
        *next_out = k_lexer_next(lexer, next_pp_kind_out);
    }

    out->swapped = false;
    if (k_token_is_ident_named(*next_out, "types")) {
        out->has_types = true;
        *next_out = k_lexer_next(lexer, next_pp_kind_out);
        /* `import M types as A;` is the clauses out of order: read it whole,
           and import-clause-order gives the right form */
        if (!out->alias.len && k_token_is_ident_named(*next_out, "as")) {
            out->alias = k_lexer_next(lexer, next_pp_kind_out);
            if (!k_token_is_ident(out->alias)) return false;
            out->swapped = true;
            *next_out = k_lexer_next(lexer, next_pp_kind_out);
        }
    }

    if (!k_token_is_punct(*next_out, ";")) return false;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return true;
}

bool k_scan_import_c(KLexer *lexer, KImportCDecl *out,
                     KToken *next_out, TKPpKind *next_pp_kind_out) {
    KToken tok = k_lexer_next(lexer, next_pp_kind_out);

    if (k_token_is_string(tok)) {
        out->header = tok;
    } else {
        if (!k_token_is_punct(tok, "<")) return false;
        KToken first = tok;
        KToken last = tok;
        while (!k_token_is_punct(last, ">")) {
            last = k_lexer_next(lexer, next_pp_kind_out);
            if (!last.len) return false;
        }
        out->header = (keel_slice_char){ .ptr = first.ptr, .len = (size_t)((last.ptr + last.len) - first.ptr) };
    }

    KToken semicolon = k_lexer_next(lexer, next_pp_kind_out);
    if (!k_token_is_punct(semicolon, ";")) return false;
    *next_out = k_lexer_next(lexer, next_pp_kind_out);
    return true;
}

/* ---- k_resolve_imports ---- */

/* Passage 1's second half (parser-design §3, [P3]): the file's imports,
   resolved into the symbol table before any declaration is collected.
   It has to be before, and not after, because keel's grammar depends on
   the table — `buffer i32 x;` is a keel declaration only once `buffer`
   is known as a modifier of arity 1 (parser-design §1). */

/* The name as written, from the first token of the path to the last:
   `keel.slice` is one slice of the source, dots included, and no
   allocation is needed for it. */
static keel_slice_char spelled(const KAst *a, size_t first, size_t end) {
    if (first >= end) return (keel_slice_char){ 0, NULL };
    KToken lead = keel_buffer_KLexeme_ptr(&a->tokens, first)->token;
    KToken last = keel_buffer_KLexeme_ptr(&a->tokens, end - 1)->token;
    return (keel_slice_char){ (size_t)((last.ptr + last.len) - lead.ptr), lead.ptr };
}

/* `types` injects the module's type, modifier and protocol names without a
   qualifier, and nothing else — keel-spec §4.1 item 6: "Não injeta
   funções, variáveis nem constantes de enum." Arity travels with a
   modifier, because that is what decides how many arguments its uses
   read. */
static bool inject_types(KSymbolTable *symtab, const KModule *module, KDiagnosticSink *diag, keel_slice_char at,
                         bool *clash) {
    for (size_t i = 0; i < module->symbol_count; i++) {
        const KSymbol *sym = &module->symbols[i];
        if (sym->kind != K_SYM_TYPE && sym->kind != K_SYM_MODIFIER && sym->kind != K_SYM_PROTOCOL) continue;
        /* two imports with `types` inject the same bare name (keel-spec §4.1):
           one of them is written without `types`, and the name qualified */
        for (size_t j = 0; j < symtab->len; j++) {
            const KSymbol *old = keel_buffer_KSymbol_ptr(symtab, j);
            if ((old->kind == K_SYM_TYPE || old->kind == K_SYM_MODIFIER || old->kind == K_SYM_PROTOCOL) &&
                old->origin && old->origin != module && k_symtab_same_name(old->name, sym->name)) {
                k_diag_emit(diag, K_DIAG_DUPLICATE_INJECTED_NAME, at,
                            (KDiagArgs){{ sym->name, module->name, old->origin->name }});
                *clash = true;
                break;
            }
        }
        if (!k_symtab_insert(symtab, sym->name, sym->kind, sym->arity)) return false;
        (*keel_buffer_KSymbol_ptr(symtab, symtab->len - 1)) = *sym;
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

    bool clash = false;
    if (has_types && !inject_types(symtab, module, diag, at, &clash)) return false;
    if (clash) return false;
    /* the names `types` brought, at the import that wrote it (injected-names,
       info: it comes out by default). The prelude's are left out: its import
       is implicit and has no position. */
    if (has_types && !k_symtab_same_name(module_name, (keel_slice_char){ 4, (char *)"keel" })) {
        static char store[32768];
        static size_t used;
        size_t start = used;
        for (size_t i = 0; i < module->symbol_count; i++) {
            const KSymbol *sym = &module->symbols[i];
            if (sym->kind != K_SYM_TYPE && sym->kind != K_SYM_MODIFIER && sym->kind != K_SYM_PROTOCOL) continue;
            if (used + sym->name.len + 2 >= sizeof store) { used = start; break; }
            if (used > start) { store[used++] = ','; store[used++] = ' '; }
            memcpy(store + used, sym->name.ptr, sym->name.len);
            used += sym->name.len;
        }
        if (used > start)
            k_diag_emit(diag, K_DIAG_INJECTED_NAMES, at,
                        (KDiagArgs){{ module_name, (keel_slice_char){ used - start, store + start } }});
    }
    /* Keep the real qualifier as well as any alias; neither changes origin. */
    if (!k_symtab_insert(symtab, module_name, K_SYM_MODULE, 0)) return false;
    keel_buffer_KSymbol_ptr(symtab, symtab->len - 1)->origin = module;
    if (alias.len != 0) {
        if (!k_symtab_insert(symtab, alias, K_SYM_MODULE, 0)) return false;
        keel_buffer_KSymbol_ptr(symtab, symtab->len - 1)->origin = module;
    }
    return true;
}

bool k_resolve_imports(const KAst *ast, KLoader *loader, KSymbolTable *symtab,
                        KDiagnosticSink *diag) {
    bool ok = true;

    /* keel-spec §4.1: every file has the implicit `import keel types;`,
       which is where `i32` and the other primitive names come from. The
       prelude itself is the one file that does not get it. */
    const KAstNode *mod = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    keel_slice_char self = spelled(ast, mod->name_first, mod->name_end);
    static const char keel_name[] = "keel";
    keel_slice_char prelude = { 4, (char *)keel_name };
    if (!k_symtab_same_name(self, prelude)) {
        if (!resolve_one(loader, symtab, diag, prelude, true,
                         (keel_slice_char){ 0, NULL }, self))
            ok = false;
    }

    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->kind != K_AST_IMPORT) continue;
        keel_slice_char name = spelled(ast, n->name_first, n->name_end);
        keel_slice_char alias = n->alias != (size_t)-1
            ? keel_buffer_KLexeme_ptr(&ast->tokens, n->alias)->token
            : (keel_slice_char){ 0, NULL };
        KToken at = keel_buffer_KLexeme_ptr(&ast->tokens, n->anchor)->token;
        if (n->clause_swapped) {
            k_diag_emit(diag, K_DIAG_IMPORT_CLAUSE_ORDER, at, (KDiagArgs){{ name, alias }});
            ok = false;
        }
        /* an alias is the qualifier of one import only, and never `keel`
           (keel-spec §4.1, duplicate-alias) */
        if (alias.len) {
            keel_slice_char other = {0};
            if (k_symtab_same_name(alias, prelude)) other = prelude;
            for (size_t j = 0; j < ast->nodes.len && !other.len; j++) {
                const KAstNode *m = keel_buffer_KAstNode_ptr(&ast->nodes, j);
                if (m->kind != K_AST_IMPORT || j == i) continue;
                keel_slice_char mname = spelled(ast, m->name_first, m->name_end);
                keel_slice_char malias = m->alias != (size_t)-1
                    ? keel_buffer_KLexeme_ptr(&ast->tokens, m->alias)->token : (keel_slice_char){0};
                if (k_symtab_same_name(alias, mname) || (j < i && malias.len && k_symtab_same_name(alias, malias)))
                    other = mname;
            }
            if (other.len) {
                k_diag_emit(diag, K_DIAG_DUPLICATE_ALIAS, alias, (KDiagArgs){{ alias, name, other }});
                ok = false;
            }
        }
        if (!resolve_one(loader, symtab, diag, name, n->has_types, alias,
                         keel_buffer_KLexeme_ptr(&ast->tokens, n->anchor)->token))
            ok = false;
    }
    return ok;
}
