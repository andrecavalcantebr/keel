/* engine/lexer/group_depth.c — the depth of one delimiter kind as a scanner
 * keeps it, per alternative of a conditional group (keel-spec §2.4;
 * lexer-design §7). No I/O, no allocation. */
#include <string.h>
#include "engine/lexer/lexer.h"

bool k_token_is_plain_else(KToken t) {
    const char *p = t.ptr, *end = t.ptr + t.len;
    if (p >= end || *p != '#') return false;
    for (p++; p < end && (*p == ' ' || *p == '\t'); p++) ;
    if (end - p < 4 || memcmp(p, "else", 4) != 0) return false;
    p += 4;
    return p == end || !((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_');
}

void k_group_depth_init(KGroupDepth *d, KGroupKind kind) {
    d->depth = 0;
    d->kind = kind;
    d->groups = 0;
    d->over = 0;
}

static void finish(KGroupDepth *d, int depth) {
    typeof(d->g[0]) *g = &d->g[d->groups - 1];
    if (!g->has_result) { g->result = depth; g->has_result = true; }
}

/* +1 opens, -1 closes, 0 is not a delimiter of `kind` (or of any, for K_GROUP_KIND_ANY) */
static int delta(KGroupKind kind, KToken t) {
    bool brace = k_token_is_punct(t, "{") || k_token_is_punct(t, "}");
    bool paren = k_token_is_punct(t, "(") || k_token_is_punct(t, ")");
    bool bracket = k_token_is_punct(t, "[") || k_token_is_punct(t, "]");
    bool mine = kind == K_GROUP_KIND_ANY ? (brace || paren || bracket)
              : kind == K_GROUP_KIND_BRACE ? brace : kind == K_GROUP_KIND_PAREN ? paren : bracket;
    if (!mine) return 0;
    return (k_token_is_punct(t, "{") || k_token_is_punct(t, "(") || k_token_is_punct(t, "[")) ? 1 : -1;
}

void k_group_depth_feed(KGroupDepth *d, KToken t, TKPpKind pp) {
    if (pp == TK_PP_IF) {
        if (d->groups == K_GROUP_DEPTH_MAX) { d->over++; return; }
        typeof(d->g[0]) *g = &d->g[d->groups++];
        g->entry = d->depth;
        g->has_result = g->has_else = false;
        return;
    }
    if (pp == TK_PP_ELSE) {
        if (d->over || !d->groups) return;
        finish(d, d->depth);
        if (k_token_is_plain_else(t)) d->g[d->groups - 1].has_else = true;
        d->depth = d->g[d->groups - 1].entry;
        return;
    }
    if (pp == TK_PP_ENDIF) {
        if (d->over) { d->over--; return; }
        if (!d->groups) return;
        finish(d, d->depth);
        typeof(d->g[0]) *g = &d->g[--d->groups];
        d->depth = g->result;
        (void)g->has_else;      /* a group without #else has an empty alternative: it agrees, or k_check_delimiters refused */
        return;
    }
    if (d->over) return;
    d->depth += delta(d->kind, t);
}
