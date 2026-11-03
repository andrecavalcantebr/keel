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

static bool alone_on_line(const Emit *e, const KAstNode *n) {
    size_t a = k_em_line_start(e, k_em_first_off(e, n)), b = k_em_line_end(e, k_em_last_end(e, n));
    for (size_t i = 0; i < e->ast->nodes.len; i++) {
        const KAstNode *m = keel_buffer_KAstNode_ptr(&e->ast->nodes, i);
        if (m != n && k_em_overlaps(e, m, a, b)) return false;
    }
    return true;
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
            break;
        case K_AST_OPAQUE:
            break;
        case K_AST_IMPORT_C:
            /* import_c <x>; (the <...> form, several tokens) or import_c "x"; (one token) */
            if (n->end - n->first < 3) return false;
            if (k_em_tok(ast, n->first + 1).ptr[0] == '"') {
                if (n->end - n->first != 3) return false;
            } else {
                if (n->end - n->first < 5) return false;
                if (k_em_tok(ast, n->first + 1).ptr[0] != '<') return false;
                if (!k_em_text_is(k_em_tok(ast, n->end - 2), ">")) return false;
            }
            if (!k_em_text_is(k_em_tok(ast, n->end - 1), ";")) return false;
            break;
        case K_AST_TYPE: {
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            /* `extent struct`: a shape of its own (backend §5.15); `pub` or `priv`, the struct goes to the
             * .type.h and the accessors to the .h (backend §5.15, rule 3) */
            if (k_em_text_is(k_em_tok(ast, k), "extent")) { if (!k_em_is_extent(ast, n)) return false; break; }
            /* `[pub|priv] struct|union NAME { … };` (decl-struct, without typedef) and `typedef`:
             * both are one node per declared name */
            if (k >= n->end) return false;
            if (!k_em_text_is(k_em_tok(ast, k), "typedef") && !k_em_text_is(k_em_tok(ast, k), "struct") &&
                !k_em_text_is(k_em_tok(ast, k), "union")) return false;
            /* a list of typedef declarators shares the span of its nodes (`typedef struct {…} P, *PP;`);
             * a struct node after another node of the same span is refused: its variables come first */
            if (i > 0) {
                const KAstNode *p = keel_buffer_KAstNode_ptr(&ast->nodes, i - 1);
                if (p->first == n->first && p->kind != K_AST_TYPE) return false;
            }
            if (!k_em_supported_struct_arrays(ast, n)) return false;
            break;
        }
        case K_AST_TAGS: {
            /* [pub|priv] tags N [A, B = 3]; both go to the .type.h (backend §5.6, rule 1); a value is a number, optionally negative */
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            if (n->name_first >= n->name_end || n->name_first != k + 1) return false;
            if (n->end - n->name_first < 4 || !k_em_text_is(k_em_tok(ast, n->name_first + 1), "[")) return false;
            for (size_t i = n->name_first + 2; i + 2 < n->end; i++) {
                if (!k_em_text_is(k_em_tok(ast, i), "=")) continue;
                size_t v = i + 1;
                if (k_em_text_is(k_em_tok(ast, v), "-")) v++;
                char c = k_em_tok(ast, v).ptr[0];
                if (c < '0' || c > '9') return false;
            }
            break;
        }
        case K_AST_CONSTEXPR: {
            /* [pub|priv] constexpr T NAME = INIT; INIT not naming the module */
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            if (n->name_first >= n->name_end || n->name_first + 2 >= n->end - 1 || n->name_first <= k + 1) return false;
            if (!k_em_text_is(k_em_tok(ast, k), "constexpr") || !k_em_text_is(k_em_tok(ast, n->name_first + 1), "=")) return false;
            if (!ast->source.len) return false;
            break;
        }
        case K_AST_VARIABLE: {
            /* several declarators (`pub int x = 1, y = 2;`): one node each, sharing the span; an
             * `array` list takes its declarators after the first as `name[` (array.c reads them) */
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            bool arr = k < n->end && k_em_text_is(k_em_tok(ast, k), "array");
            size_t depth = 0;
            for (size_t p = n->first; p < n->end && arr; p++) {
                KToken t = k_em_tok(ast, p);
                if (k_em_text_is(t, "(") || k_em_text_is(t, "[") || k_em_text_is(t, "{")) depth++;
                else if (k_em_text_is(t, ")") || k_em_text_is(t, "]") || k_em_text_is(t, "}")) depth--;
                else if (depth == 0 && k_em_text_is(t, ",") &&
                         !(p + 2 < n->end && k_em_text_is(k_em_tok(ast, p + 2), "["))) return false;
            }
            break;
        }
        case K_AST_EXTERN_C: {
            size_t k = n->first + (k_em_is_visibility(k_em_tok(ast, n->first)) ? 1 : 0);
            if (k + 1 >= n->end || !k_em_text_is(k_em_tok(ast, k), "extern_c")) return false;
            if (k_em_text_is(k_em_tok(ast, k + 1), "[")) {
                if (k + 4 >= n->end || !k_em_text_is(k_em_tok(ast, k + 2), "type_h") ||
                    !k_em_text_is(k_em_tok(ast, k + 3), "]") || !k_em_text_is(k_em_tok(ast, k + 4), "{")) return false;
            } else {
                /* the plain form (.h, with or without `pub`) and `priv` (.c) */
                if (!k_em_text_is(k_em_tok(ast, k + 1), "{")) return false;
            }
            if (n->body_first == (size_t)-1 || n->body_end == (size_t)-1) return false;
            /* one line: its text is a line of its own, and no other construction may share it */
            if (k_em_extern_c_inline(ast, n) && !alone_on_line(e, n)) return false;
            break;
        }
        case K_AST_FUNCTION:
            /* a forward declaration has no body: `priv void release(R *r);` is a
             * line of the .c like any other (backend §4.1). A `pub` one would
             * have to be weighed against the prototype the .h already writes,
             * and has no golden. */
            if (n->body_first == (size_t)-1) {
                if (n->is_public || !k_em_text_is(k_em_tok(ast, n->end - 1), ";")) return false;
                break;
            }
            if (n->body_first < 2) return false;
            if (!k_em_text_is(k_em_tok(ast, n->body_first - 1), "{") || !k_em_text_is(k_em_tok(ast, n->body_first - 2), ")"))
                return false;
            if (!k_em_supported_constblock(e, n)) return false;
            break;
        default:
            return false;
        }
    }
    return true;
}
