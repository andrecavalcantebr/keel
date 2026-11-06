/* engine/emit/supported.c — the test of the slice: whether the emitter covers every
 * island and every node of a module (emit_internal.h; backend §4.1, §5). A module it
 * does not cover is K_EMIT_UNSUPPORTED, never a mistranslation. No I/O, no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/names.h"
#include "engine/symtab.h"

/* whether island `is` sits inside a declaration that is out (its anchor token) */
static bool island_out(const KAst *ast, const KIsland *is) {
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (is->anchor >= n->first && is->anchor < n->end && k_em_generic_out(ast, n)) return true;
    }
    return false;
}

/* Whether the islands of the module are the ones a rename covers: a type,
 * a name or a call whose detail is `<name>[/arity] → <symbol>` and whose name
 * is declared by this module. Any other island is a construction that needs
 * lowering (backend §5), which the slice does not have. */
static bool islands_supported(const KAst *ast) {
    for (size_t i = 0; i < ast->islands.len; i++) {
        const KIsland *is = keel_buffer_KIsland_ptr(&ast->islands, i);
        if (island_out(ast, is)) continue;   /* in a declaration of the instance */
        if (is->kind != K_ISLAND_TYPE && is->kind != K_ISLAND_NAME && is->kind != K_ISLAND_CALL) {
            /* a construction a file of its own lowers answers for its kind; the
             * rename check below does not apply to it */
            if (k_em_handles_array(ast, is) || k_em_handles_foreach(ast, is) ||
                k_em_handles_index(ast, is) || k_em_handles_constblock(ast, is) ||
                k_em_handles_defer(ast, is) || k_em_handles_implicit_init(ast, is) || k_em_handles_ref(ast, is) ||
                k_em_handles_else(ast, is) || k_em_handles_subscript(ast, is) || k_em_handles_match(ast, is) ||
                k_em_handles_parallel(ast, is) || k_em_handles_worker_exit(ast, is) ||
                is->kind == K_ISLAND_EXTENT || k_em_handles_column(ast, is)) continue;
            return false;
        }
        /* a name another module declares: renamed by its island (uses.c) */
        if (k_em_foreign_island(ast, is)) {
            if (!k_em_handles_use(ast, is) && !k_em_handles_core(ast, is)) return false;
            continue;
        }
        const char *d = keel_buffer_char_ptr(&ast->island_text, is->text);
        size_t len = is->text_len, nm = 0;
        while (nm < len && d[nm] != '/' && d[nm] != ' ') nm++;
        bool arrow = false;
        for (size_t k = nm; k + 3 <= len; k++)
            if (memcmp(d + k, "\xE2\x86\x92", 3) == 0) { arrow = true; break; }
        if (!arrow) return false;
        bool declared = false;
        for (size_t k = 0; k < ast->nodes.len && !declared; k++) {
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            if (n->name_first >= n->name_end) continue;
            KToken t = k_em_tok(ast, n->name_first);
            declared = t.len == nm && memcmp(t.ptr, d, nm) == 0;
        }
        if (!declared) return false;
    }
    return true;
}

/* Whether the slice of emit.h covers every node of the module. */
bool k_em_supported(const Emit *e) {
    const KAst *ast = e->ast;
    if (!islands_supported(ast)) return false;
    if (k_em_places_instances(ast)) return k_em_supported_placement(ast);
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->first >= n->end) return false;
        if (k_em_generic_out(ast, n)) continue;
        switch (n->kind) {
        case K_AST_MODULE:
        case K_AST_PROTOCOL:
        case K_AST_IMPORT:   /* emits nothing: the #include comes from the uses (backend §4.3.2, rule 4) */
        case K_AST_OPAQUE:
            break;
        case K_AST_IMPORT_C:
            if (!k_em_handles_import_c(ast, n)) return false;
            break;
        case K_AST_TYPE:
            if (!k_em_handles_type(ast, n, i > 0 ? keel_buffer_KAstNode_ptr(&ast->nodes, i - 1) : NULL)) return false;
            break;
        case K_AST_TAGS:
            if (!k_em_handles_tags(ast, n)) return false;
            break;
        case K_AST_CONSTEXPR:
            if (!k_em_handles_constexpr(ast, n)) return false;
            break;
        case K_AST_VARIABLE:
            if (!k_em_handles_variable(ast, n)) return false;
            break;
        case K_AST_EXTERN_C:
            if (!k_em_handles_extern_c(e, n)) return false;
            break;
        case K_AST_FUNCTION:
            if (!k_em_handles_function(e, n)) return false;
            break;
        default:
            return false;
        }
    }
    return true;
}
