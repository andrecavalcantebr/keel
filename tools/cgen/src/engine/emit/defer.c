/* engine/emit/defer.c — `defer` of a function body (emit_internal.h; codegen
 * design E5, backend §5.5). Empty for now: its hooks answer "nothing here" until
 * the lowering lands. */
#include "engine/emit/emit_internal.h"

void k_em_edits_defer(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    (void)e; (void)fn; (void)a; (void)b; (void)ed; (void)cnt;
}

bool k_em_handles_defer(const KAst *ast, const KIsland *is) {
    (void)ast; (void)is;
    return false;
}
