/* engine/emit/constexpr_block.c — the block-scope `constexpr` of a function body
 * (emit_internal.h; codegen design E5, backend §9.2). Empty for now: the edits
 * and the lines before and after are the hooks of emit.c, and the C11 answer
 * refuses a function whose body holds a `constexpr` until the lowering lands. */
#include "engine/emit/emit_internal.h"

void k_em_edits_constblock(Emit *e, const KAstNode *fn, size_t a, size_t b, Edit *ed, size_t *cnt) {
    (void)e; (void)fn; (void)a; (void)b; (void)ed; (void)cnt;
}

bool k_em_handles_constblock(const KAst *ast, const KIsland *is) {
    (void)ast; (void)is;
    return false;
}

void k_em_pre_lines_constblock(Emit *e, size_t ln, size_t a, size_t b) {
    (void)e; (void)ln; (void)a; (void)b;
}

void k_em_post_lines_constblock(Emit *e, size_t ln, size_t a, size_t b) {
    (void)e; (void)ln; (void)a; (void)b;
}

/* C23 takes the block constexpr verbatim, so every function is supported. C11
 * has no lowering yet: a `constexpr` token in the body makes the call
 * UNSUPPORTED (refused, not copied into invalid C). */
bool k_em_supported_constblock(const Emit *e, const KAstNode *fn) {
    if (e->opt.c23) return true;
    for (size_t i = fn->body_first; i < fn->body_end; i++)
        if (k_em_text_is(k_em_tok(e->ast, i), "constexpr")) return false;
    return true;
}

bool k_em_constblock_rename(const Emit *e, size_t tok, char *out, size_t cap, size_t *len) {
    (void)e; (void)tok; (void)out; (void)cap; (void)len;
    return false;
}
