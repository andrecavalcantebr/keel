/* engine/emit/else.c — the `else` of a result (emit_internal.h; codegen design
 * E5: one file per construction; spec §4.10, backend §5.12). To be written by
 * the card M6a (tools/agents/tasks/M6a-else.md): until then it lowers nothing,
 * and a module with an `else` island is refused with implementation-limit. */
#include "engine/emit/emit_internal.h"

void k_em_edits_else(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    (void)e; (void)fn; (void)a; (void)b; (void)ed; (void)cnt;
}

bool k_em_handles_else(const KAst *ast, const KIsland *is) {
    (void)ast; (void)is;
    return false;
}
