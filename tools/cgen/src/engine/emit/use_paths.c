/* engine/emit/use_paths.c — the header of a symbol another module owns: the
 * imported module's, or an instance's (backend §4.3.2, rules 2, 3 and 4). Split
 * from uses.c, whose rename of a use (Detail, detail_of) this does not need: a
 * self-contained "which header does this symbol live in" library that
 * forward_decls, h_type_includes, use_includes, h_body_includes, type_h_includes
 * and instance_type_includes (uses.c) share. No I/O, no allocation. */
#include <string.h>
#include "engine/emit/emit_internal.h"
#include "engine/names.h"
#include "engine/symtab.h"

/* ---- the includes (rule 3, section 1; rule 4) ---- */

static bool prefix_of(const char *sym, size_t len, const char *p, size_t plen) {
    return len == plen ? memcmp(sym, p, plen) == 0 : len > plen && memcmp(sym, p, plen) == 0 && sym[plen] == '_';
}

/* the instance of the closure the symbol belongs to (the longest), or -1 */
static int instance_of(const KAst *ast, const char *sym, size_t len) {
    int best = -1;
    size_t best_len = 0;
    for (size_t c = 0; c < ast->closure.len; c++) {
        const KClosure *k = keel_buffer_KClosure_ptr(&ast->closure, c);
        if (k->kind != K_CLOSURE_INSTANCE) continue;
        if (prefix_of(sym, len, k->symbol, k->symbol_len) && k->symbol_len > best_len) { best = (int)c; best_len = k->symbol_len; }
        /* the unit of a module instance (`keel_routine_ag2_Ag_seq`) */
        char unit[NAME_CAP];
        size_t ul = k_em_unit_symbol(ast, k, unit, sizeof unit);
        if (ul && prefix_of(sym, len, unit, ul) && ul > best_len) { best = (int)c; best_len = ul; }
    }
    return best;
}

/* the dotted name of an import node, into buf */
static keel_slice_char import_name(const KAst *ast, const KAstNode *n, char *buf, size_t cap) {
    size_t len = 0;
    for (size_t j = n->name_first; j < n->name_end; j++) {
        KToken t = k_em_tok(ast, j);
        if (len + t.len > cap) return (keel_slice_char){ 0 };
        memcpy(buf + len, t.ptr, t.len);
        len += t.len;
    }
    return (keel_slice_char){ len, buf };
}

/* The symbol of the protocol-function instance a verb symbol belongs to: the verbs of
 * `keel_slice_of_keel_array_i16` are `..of`, `..of1` and `..of2` (the arity digits
 * follow the verb), and all live in the instance's one header. The instance's symbol,
 * or the symbol itself when no protocol function of the closure matches. */
static const char *proto_family(const KAst *ast, const char *sym, size_t len, size_t *out_len) {
    for (size_t c = 0; c < ast->closure.len; c++) {
        const KClosure *k = keel_buffer_KClosure_ptr(&ast->closure, c);
        if (k->kind != K_CLOSURE_PROTOCOL_FUNCTION || k->symbol_len > len) continue;
        size_t i = 0;
        while (i < k->symbol_len && sym[i] == k->symbol[i]) i++;
        if (i == k->symbol_len && len == i) break;                       /* the instance's own symbol */
        size_t j = i;
        while (j < len && sym[j] >= '0' && sym[j] <= '9') j++;
        if (j > i && sym[i - 1] != '_' && len - j == k->symbol_len - i && memcmp(sym + j, k->symbol + i, len - j) == 0) {
            *out_len = k->symbol_len;
            return k->symbol;
        }
    }
    *out_len = len;
    return sym;
}

/* The header a symbol of another module lives in, with extension `ext`: its
 * instance's, or the imported module's whose symbol prefixes it (the longest).
 * Its length in path, 0 when none (the prelude's names are everywhere). */
size_t k_em_header_of(const KAst *ast, const char *sym, size_t len, const char *ext, char *path, size_t cap) {
    int c = instance_of(ast, sym, len);
    if (c >= 0) {
        const KClosure *k = keel_buffer_KClosure_ptr(&ast->closure, (size_t)c);
        if (!k->module) return 0;
        char unit[NAME_CAP];
        size_t ul = k_em_unit_symbol(ast, k, unit, sizeof unit);
        size_t pl = k_mangle_instance_path(k->module->name, ul ? (keel_slice_char){ ul, unit } : (keel_slice_char){ k->symbol_len, (char *)k->symbol }, ext,
                                           (keel_slice_char){ cap, path });
        return pl <= cap ? pl : 0;
    }
    size_t best = 0;
    char name_buf[NAME_CAP], best_name[NAME_CAP];
    size_t best_len = 0;
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_IMPORT) continue;
        keel_slice_char name = import_name(ast, n, name_buf, sizeof name_buf);
        if (!name.len || k_em_is_keel(name)) continue;
        char ms[NAME_CAP];
        size_t ml = k_mangle_module(name, (keel_slice_char){ sizeof ms, ms });
        if (ml > sizeof ms || !prefix_of(sym, len, ms, ml) || ml <= best) continue;
        best = ml;
        memcpy(best_name, name.ptr, name.len);
        best_len = name.len;
    }
    /* the module itself: an instance reaches the generic's share (backend §4.4.1, rule 4) */
    const KAstNode *mn = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    keel_slice_char own = import_name(ast, mn, name_buf, sizeof name_buf);
    if (own.len && !k_em_is_keel(own)) {
        char ms[NAME_CAP];
        size_t ml = k_mangle_module(own, (keel_slice_char){ sizeof ms, ms });
        if (ml <= sizeof ms && prefix_of(sym, len, ms, ml) && ml > best) {
            best = ml;
            memcpy(best_name, own.ptr, own.len);
            best_len = own.len;
        }
    }
    if (!best) return 0;
    /* a generic module's symbol that none of its declarations has: one of its instances */
    const KModule *gm = NULL;
    for (size_t i = 0; ast->symbols && i < ast->symbols->len && !gm; i++) {
        const KSymbol *q = keel_buffer_KSymbol_ptr(ast->symbols, i);
        if (q->kind == K_SYM_MODULE && q->origin && q->origin->ast && q->origin->name.len == best_len &&
            memcmp(q->origin->name.ptr, best_name, best_len) == 0) gm = q->origin;
    }
    if (!gm && own.len && best_len == own.len && memcmp(best_name, own.ptr, own.len) == 0) {
        /* the module itself: its generic tree is this one */
        static KModule self;
        self = (KModule){ .name = own, .ast = ast };
        gm = &self;
    }
    if (gm && k_em_module_is_generic(gm->ast)) {
        bool share = false;
        for (size_t k = 0; k < gm->symbol_count && !share; k++) {
            char ms[NAME_CAP];
            size_t ml = k_mangle_symbol(&gm->symbols[k], (keel_slice_char){ sizeof ms, ms });
            share = ml == len && memcmp(ms, sym, len) == 0;
        }
        for (size_t k = 0; gm->ast == ast && k < ast->nodes.len && !share; k++) {   /* the module's own share */
            const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
            if (n->name_first >= n->name_end || k_em_generic_out(ast, n)) continue;
            char ms[NAME_CAP];
            KToken t = k_em_tok(ast, n->name_first);
            size_t ml = k_mangle_decl(own, (keel_slice_char){ t.len, (char *)t.ptr }, n->kind == K_AST_TYPE || n->kind == K_AST_TAGS,
                                      (keel_slice_char){ sizeof ms, ms });
            share = ml == len && memcmp(ms, sym, len) == 0;
        }
        if (!share) {
            size_t fl;
            const char *fam = proto_family(ast, sym, len, &fl);
            size_t pl = k_mangle_instance_path((keel_slice_char){ best_len, best_name }, (keel_slice_char){ fl, (char *)fam }, ext,
                                               (keel_slice_char){ cap, path });
            return pl <= cap ? pl : 0;
        }
    }
    size_t pl = k_mangle_path((keel_slice_char){ best_len, best_name }, ext, (keel_slice_char){ cap, path });
    return pl <= cap ? pl : 0;
}

void k_em_raw_include(Emit *e, const char *path, size_t len) {
    k_emit_raw(&e->w, (keel_slice_char){ 10, (char *)"#include \"" });
    k_emit_raw(&e->w, (keel_slice_char){ len, (char *)path });
    k_emit_raw(&e->w, (keel_slice_char){ 2, (char *)"\"\n" });
}

bool k_em_add_include(Includes *s, const char *path, size_t len) {
    for (size_t k = 0; k < s->n; k++) if (s->len[k] == len && memcmp(s->path[k], path, len) == 0) return true;
    if (s->n == MAX_INCLUDES) return false;
    memcpy(s->path[s->n], path, len);
    s->len[s->n++] = len;
    return true;
}


/* .h, section 1: the .type.h of each type of another module a public signature
 * takes by value, in the order of the first use (codegen design E2) */
/* the generic's own .type.h, which an instance's .h includes ahead (emit.c) */
size_t k_em_own_type_path(const Emit *e, char *path, size_t cap) {
    if (!e->inst) return 0;
    size_t pl = k_mangle_path(e->module, ".type.h", (keel_slice_char){ cap, path });
    return pl <= cap ? pl : 0;
}

/* the place of the import whose module prefixes `sym` (the longest), among the
 * imports in the order written; one past the last when none does */
size_t k_em_import_rank(const KAst *ast, const char *sym, size_t len) {
    size_t rank = 1, best = SIZE_MAX, best_len = 0;
    char name_buf[NAME_CAP];
    /* the module itself first: its line comes before the imports */
    const KAstNode *mn = keel_buffer_KAstNode_ptr(&ast->nodes, ast->module);
    keel_slice_char own = import_name(ast, mn, name_buf, sizeof name_buf);
    if (own.len) {
        char ms[NAME_CAP];
        size_t ml = k_mangle_module(own, (keel_slice_char){ sizeof ms, ms });
        if (ml <= sizeof ms && prefix_of(sym, len, ms, ml)) { best = 0; best_len = ml; }
    }
    for (size_t k = 0; k < ast->nodes.len; k++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, k);
        if (n->kind != K_AST_IMPORT) continue;
        keel_slice_char name = import_name(ast, n, name_buf, sizeof name_buf);
        char ms[NAME_CAP];
        size_t ml = name.len ? k_mangle_module(name, (keel_slice_char){ sizeof ms, ms }) : 0;
        if (ml && ml <= sizeof ms && prefix_of(sym, len, ms, ml) && ml > best_len) { best = rank; best_len = ml; }
        rank++;
    }
    return best == SIZE_MAX ? rank : best;
}

/* an argument of the instance being written that is a C type (`i32`, `const char`):
 * no module, so no header of keel */
bool k_em_plain_arg(const Emit *e, const char *sym, size_t len) {
    for (size_t k = 0; e->inst && k < e->inst->argc; k++)
        if (!e->inst->arg_module[k].len && e->inst->arg[k].len == len && memcmp(e->inst->arg[k].ptr, sym, len) == 0) return true;
    return false;
}

/* an argument of the instance being written that is itself an instance: its header */
size_t k_em_arg_header(const Emit *e, const char *sym, size_t len, const char *ext, char *path, size_t cap) {
    for (size_t k = 0; e->inst && k < e->inst->argc; k++) {
        keel_slice_char a = e->inst->arg[k];
        if (!e->inst->arg_module[k].len || a.len != len || memcmp(a.ptr, sym, len) != 0) continue;
        size_t pl = k_mangle_instance_path(e->inst->arg_module[k], k_emit_arg_file(e->inst, k), ext, (keel_slice_char){ cap, path });
        if (k_em_binder_use(e, k) == 2) return SIZE_MAX;   /* the instance's own .type.h includes it */
        return pl <= cap ? pl : 0;
    }
    return 0;
}

/* whether `sym` is the instance being written, or one of its verbs */
bool k_em_own_instance(const Emit *e, const char *sym, size_t len) {
    return e->inst && (prefix_of(sym, len, e->inst->symbol.ptr, e->inst->symbol.len) ||
                       (e->inst->unit_len && prefix_of(sym, len, e->inst->unit_buf, e->inst->unit_len)) ||
                       k_em_own_declaration(e, sym, len));
}

/* whether `sym` is the symbol of a `tags` set: an `enum`, which C cannot declare ahead
 * (`typedef struct X X;` would name another type), so a pointer to it needs the definition */
bool k_em_sym_is_tags_named(const KAst *ast, const char *sym, size_t len, const char *written, size_t wlen) {
    /* the written name (`corot.Status`) may be known to the table only qualified */
    const KSymbol *w = written && ast->symbols ? k_symbol_resolve(ast->symbols, (keel_slice_char){ wlen, (char *)written }) : NULL;
    if (w && w->kind == K_SYM_TAGS) return true;
    for (size_t i = 0; ast->symbols && i < ast->symbols->len; i++) {
        const KSymbol *s = keel_buffer_KSymbol_ptr(ast->symbols, i);
        char ms[NAME_CAP];
        if (s->kind != K_SYM_TAGS) continue;
        size_t ml = k_mangle_symbol(s, (keel_slice_char){ sizeof ms, ms });
        if (ml == len && memcmp(ms, sym, len) == 0) return true;
    }
    return false;
}
