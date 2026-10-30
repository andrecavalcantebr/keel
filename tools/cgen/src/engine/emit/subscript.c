/* engine/emit/subscript.c — the index sugar over a container (emit_internal.h;
 * codegen design E5: one file per construction; backend §5.3): `x[i]` over a
 * modifier instance is `*ptr(x, i)`, and `x[a..b]` with its open forms is
 * `as_slice(x, a, b)`. To be written by the card M6c
 * (tools/agents/tasks/M6c-subscript.md): until then it lowers nothing. */
#include "engine/emit/emit_internal.h"

void k_em_edits_subscript(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    (void)e; (void)fn; (void)a; (void)b; (void)ed; (void)cnt;
}

bool k_em_handles_subscript(const KAst *ast, const KIsland *is) {
    (void)ast; (void)is;
    return false;
}
