/* engine/emit/emit.c — the module emitter's dispatcher (emit.h; codegen design E5;
 * backend §4.1, §4.3, §5, §6). k_em_node_edits asks each file of the emitter for
 * the edits a node makes in a source span, in the order the output needs: the
 * declaration (decls.c) and the constructions of a function body (one file
 * each). The files are assembled in files.c, the partition of a generic module
 * is in generic.c, the lines are written in lines.c, and the slice is tested in
 * supported.c. No I/O, no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/names.h"
#include "engine/symtab.h"


void k_em_node_edits(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    size_t entry = *cnt;
    if (!k_em_decl_edits(e, n, a, b, ed, cnt)) return;
    /* the constructions of a function body (backend §5): each file adds its edits
     * in [a, b); the new ones are sorted in among the rename edits of this node */
    if (n->kind == K_AST_FUNCTION) {
        size_t before = *cnt;
        k_em_edits_roles(e, n, a, b, ed, cnt);
        k_em_edits_range_check(e, n, a, b, ed, cnt);
        k_em_edits_array(e, n, a, b, ed, cnt);
        k_em_edits_foreach(e, n, a, b, ed, cnt);
        k_em_edits_constblock(e, n, a, b, ed, cnt);
        k_em_edits_defer(e, n, a, b, ed, cnt);
        k_em_edits_index(e, n, a, b, ed, cnt);
        k_em_edits_else(e, n, a, b, ed, cnt);
        k_em_edits_match(e, n, a, b, ed, cnt);
        k_em_edits_parallel(e, n, a, b, ed, cnt);
        k_em_edits_worker_exit(e, n, a, b, ed, cnt);
        k_em_edits_subscript(e, n, a, b, ed, cnt);
        k_em_edits_rangelit(e, n, a, b, ed, cnt);
        k_em_edits_routine(e, n, a, b, ed, cnt);
        k_em_edits_column(e, n, a, b, ed, cnt);
        k_em_edits_core(e, n, a, b, ed, cnt);
        k_em_sort_edits(ed, before, *cnt);
    }
    /* an `array` variable at file scope (array.c) */
    if (n->kind == K_AST_VARIABLE) {
        size_t before = *cnt;
        k_em_edits_array(e, n, a, b, ed, cnt);
        k_em_edits_routine(e, n, a, b, ed, cnt);
        k_em_sort_edits(ed, before, *cnt);
    }
    /* the uses of another module's names, by their islands (uses.c) */
    if (n->kind != K_AST_MODULE && n->kind != K_AST_IMPORT) k_em_edits_uses(e, n, a, b, ed, cnt);
    /* a default whose value starts a line of its own: after uses.c, which drops the edits in a call it rewrites */
    if (n->kind == K_AST_FUNCTION) {
        size_t before = *cnt;
        k_em_edits_else_tail(e, n, a, b, ed, cnt);
        k_em_sort_edits(ed, before, *cnt);
    }
    /* the `array` fields of a struct (array.c); an extent's columns are its own (extent.c) */
    if (n->kind == K_AST_TYPE && !k_em_is_extent(ast, n)) {
        k_em_edits_struct_arrays(e, n, a, b, ed, cnt);
        k_em_sort_edits(ed, entry, *cnt);
    }
    /* the prelude's static_assert in C11 (backend §4.2, §9): only the keyword changes */
    if (n->kind == K_AST_OPAQUE && !e->opt.c23 && k_em_text_is(k_em_tok(ast, n->first), "static_assert")) {
        size_t at = k_em_tok_off(ast, n->first);
        if (at >= a && at < b)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_ASSERT, .at = at, .skip = k_em_tok(ast, n->first).len, .name_tok = 0, .is_type = false });
    }
}
