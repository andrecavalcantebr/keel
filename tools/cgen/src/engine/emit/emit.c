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


/* The analyzers of a function body (backend §5), in the order their edits are wanted. Each one
 * scans the whole body for its own construction and keeps the edits that fall in [a, b), so a
 * function of L lines costs L scans per analyzer. Most functions hold no construction of most
 * kinds: the first time a function is asked for, each analyzer is run once over the whole
 * source (a throwaway Edit array; the Emit state it may touch is put back), and one that finds
 * nothing is skipped for every line of that function (achados 2026-11-25, A4b). An analyzer
 * only clips what it finds to [a, b), so what it finds in a part it also finds in the whole;
 * a build with -DCGEN_CHECK_IDLE refuses where that fails, and the suite is run with it. */
typedef void (*Analyzer)(Emit *, const KAstNode *, size_t, size_t, Edit *, size_t *);
static const Analyzer fn_analyzers[] = {
    k_em_edits_roles, k_em_edits_range_check, k_em_edits_array, k_em_edits_foreach,
    k_em_edits_constblock, k_em_edits_defer, k_em_edits_index, k_em_edits_else, k_em_edits_match,
    k_em_edits_parallel, k_em_edits_worker_exit, k_em_edits_subscript, k_em_edits_rangelit,
    k_em_edits_routine, k_em_edits_column, k_em_edits_core,
};
enum { N_ANALYZERS = sizeof fn_analyzers / sizeof fn_analyzers[0] };

static bool analyzer_is_idle(Emit *e, const KAstNode *fn, unsigned j) {
    if (e->idle_fn != fn) { e->idle_fn = fn; e->idle_probed = 0; e->idle_mask = 0; }
    if (!((e->idle_probed >> j) & 1u)) {
        Edit probe[MAX_EDITS];
        size_t cnt = 0;
        bool ok = e->ok;
        size_t limit = e->limit, limit_line = e->limit_line, refuse_tok = e->refuse_tok, n = e->n, scratch_n = e->scratch_n;
        const char *refuse_what = e->refuse_what;
        const char *cap = k_em_take_cap();
        fn_analyzers[j](e, fn, 0, e->ast->source.len, probe, &cnt);
        bool idle = cnt == 0 && e->ok == ok && e->limit == limit && e->refuse_tok == refuse_tok;
        e->ok = ok; e->limit = limit; e->limit_line = limit_line; e->refuse_tok = refuse_tok;
        e->refuse_what = refuse_what; e->n = n; e->scratch_n = scratch_n;
        k_em_note_cap(cap);
        e->idle_probed |= 1u << j;
        if (idle) e->idle_mask |= 1u << j;
    }
    return (e->idle_mask >> j) & 1u;
}

void k_em_node_edits(Emit *e, const KAstNode *n, size_t a, size_t b, Edit *ed, size_t *cnt) {
    const KAst *ast = e->ast;
    if (n->is_draft) {
        size_t start = k_em_first_off(e, n), end = k_em_last_end(e, n);
        if (start < a) start = a;
        if (end >= b) end = b - (b > a && ast->source.ptr[b - 1] == '\n');
        if (end > start)
            k_em_push_edit(e, ed, cnt, (Edit){ .kind = EDIT_DROP, .at = start, .skip = end - start });
        return;
    }
    size_t entry = *cnt;
    if (!k_em_decl_edits(e, n, a, b, ed, cnt)) return;
    /* the constructions of a function body (backend §5): each file adds its edits
     * in [a, b); the new ones are sorted in among the rename edits of this node */
    if (n->kind == K_AST_FUNCTION) {
        size_t before = *cnt;
        for (unsigned j = 0; j < N_ANALYZERS; j++) {
            if (analyzer_is_idle(e, n, j)) {
#ifdef CGEN_CHECK_IDLE   /* a build that proves the probe: an idle analyzer must stay idle on every line */
                Edit probe[MAX_EDITS];
                size_t pc = 0;
                fn_analyzers[j](e, n, a, b, probe, &pc);
                if (pc != 0) k_em_refuse(e, n->first, "an analyzer judged idle made edits (CGEN_CHECK_IDLE)");
#endif
                continue;
            }
            fn_analyzers[j](e, n, a, b, ed, cnt);
        }
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
