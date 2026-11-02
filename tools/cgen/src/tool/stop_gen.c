/* tool/stop_gen.c — `--stop-after=gen` (cgen design §5.3; cgen-tool-spec
 * §4.2): load the module and the prelude, run the emitter (engine/emit/emit.c)
 * once per generated file, and write each under --dest-dir. The files follow
 * the writing rules of §6 (cgen_write_output: an identical file is not touched).
 * The slice of the language the emitter covers is its own (emit.h); a module
 * outside it is refused here, never half-written. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "tool/tool.h"
#include "tool/cli_limits.h"
#include "engine/emit/emit.h"
#include "engine/emit/mangle.h"
#include "engine/islands.h"

int cgen_write_output(const char *path, const char *data, size_t n);

static bool absolute(const char *path, char *out, size_t cap) {
    if (path[0] == '/') return cgen_path_normalize(path, out, cap);
    char cwd[4096], joined[8192];
    if (!getcwd(cwd, sizeof cwd)) return false;
    int n = snprintf(joined, sizeof joined, "%s/%s", cwd, path);
    return n >= 0 && (size_t)n < sizeof joined && cgen_path_normalize(joined, out, cap);
}

/* cgen-tool-spec §4.9: --profile, else the -std= among the passed-through options */
static bool profile_is_c23(const char *profile, const char *const *pass, int pass_n) {
    if (profile && strcmp(profile, "c23") == 0) return true;
    if (profile && strcmp(profile, "c11") == 0) return false;
    for (int i = 0; i < pass_n; i++) {
        if (strncmp(pass[i], "-std=", 5) != 0) continue;
        const char *v = pass[i] + 5;
        return strcmp(v, "c23") == 0 || strcmp(v, "gnu23") == 0 ||
               strcmp(v, "c2x") == 0 || strcmp(v, "gnu2x") == 0;
    }
    return false;
}

/* cgen-tool-spec §4.8: --parallel-lowering; `auto` (and no option) reads -fopenmp among the
 * passed-through options. `openmp` without it was refused by the caller. */
static bool parallel_is_omp(const char *lowering, const char *const *pass, int pass_n) {
    if (lowering && strcmp(lowering, "serie") == 0) return false;
    if (lowering && strcmp(lowering, "openmp") == 0) return true;
    for (int i = 0; i < pass_n; i++)
        if (strcmp(pass[i], "-fopenmp") == 0 || strncmp(pass[i], "-fopenmp=", 9) == 0) return true;
    return false;
}

/* mkdir -p of the directories above `path` */
static bool make_parents(const char *path) {
    char buf[8192];
    size_t len = strlen(path);
    if (len >= sizeof buf) return false;
    memcpy(buf, path, len + 1);
    for (size_t i = 1; i < len; i++) {
        if (buf[i] != '/') continue;
        buf[i] = '\0';
        if (mkdir(buf, 0777) != 0 && errno != EEXIST) return false;
        buf[i] = '/';
    }
    return true;
}

static int write_failed(const char *path) {
    fprintf(stderr, "cgen: error: cannot write '%s' [write-failure]\n", path);
    return 2;
}

/* One generated file: measure, fill, create the directories, write. */
static int emit_one(keel_arena *arena, const char *dest, const KAst *ast, keel_slice_char module,
                    keel_slice_char src_path, KEmitFile which, const char *ext, KEmitOptions opt) {
    size_t need = k_emit_file(ast, module, src_path, which, opt, (keel_slice_char){0});
    if (need == K_EMIT_UNSUPPORTED) {
        fprintf(stderr, "cgen: error: %.*s: the emitter does not cover this module's constructions yet [implementation-limit]\n",
                (int)src_path.len, src_path.ptr);
        return 2;
    }
    char *data = cgen_alloc(arena, need ? need : 1, 1, 1, false, "generated file");
    if (!data) return 2;
    k_emit_file(ast, module, src_path, which, opt, (keel_slice_char){ need, data });

    char rel[4096];
    size_t rel_len;
    if (which == K_EMIT_ENTRY) rel_len = k_mangle_entry(module, (keel_slice_char){ sizeof rel, rel });
    else rel_len = k_mangle_path(module, ext, (keel_slice_char){ sizeof rel, rel });
    char path[8192];
    if (rel_len >= sizeof rel) return write_failed(dest);
    int n = snprintf(path, sizeof path, "%s/%.*s", dest, (int)rel_len, rel);
    if (n < 0 || (size_t)n >= sizeof path || !make_parents(path)) return write_failed(dest);
    return cgen_write_output(path, data, need);
}

/* The .k path of a module, for `#line` and the header comment: its name with
 * `/` for each `.`, and `.k` (backend §4.1: the file is the symbol). */
static keel_slice_char source_path(keel_slice_char name, char *buf, size_t cap) {
    if (name.len + 2 > cap) return (keel_slice_char){ 0 };
    for (size_t i = 0; i < name.len; i++) buf[i] = name.ptr[i] == '.' ? '/' : name.ptr[i];
    memcpy(buf + name.len, ".k", 2);
    return (keel_slice_char){ name.len + 2, buf };
}

/* the modules whose headers are written in this call, each once */
#define WRITTEN_CAP 1024
typedef struct {
    const KModule *seen[WRITTEN_CAP];
    size_t n;
} Written;

static keel_slice_char name_span(const KAst *a, size_t first, size_t end) {
    KToken x = keel_buffer_KLexeme_ptr(&a->tokens, first)->token, y = keel_buffer_KLexeme_ptr(&a->tokens, end-1)->token;
    return (keel_slice_char){ (size_t)(y.ptr + y.len - x.ptr), x.ptr };
}

/* The headers (.type.h and .h, no .c) of the modules `m` imports, and of what
 * they import, each module once (backend §4.3.2, rule 4: the uses carry the
 * #include, not the imports). The prelude and the compiled module are in `w`
 * already. A generic module writes what the emitter says it owns (backend
 * §4.4.1). */
static int emit_imports(keel_arena *arena, void *tool, const char *dest, const KModule *m,
                        Written *w, KEmitOptions opt, bool base) {
    const KAst *ast = m->ast;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->kind != K_AST_IMPORT) continue;
        KModule *dep = NULL;
        KLoadResult r = cgen_load(tool, name_span(ast, n->name_first, n->name_end), &dep);
        if ((r != K_LOAD_OK && r != K_LOAD_ALREADY) || !dep) return 2;
        /* the base's modules (keel.*) are written with the base, not by their users */
        keel_slice_char dn = dep->name;
        if (!base && dn.len > 5 && memcmp(dn.ptr, "keel.", 5) == 0) continue;
        if (dn.len == 4 && memcmp(dn.ptr, "keel", 4) == 0) continue;   /* the prelude is written by the caller */
        bool done = false;
        for (size_t k = 0; k < w->n; k++) if (w->seen[k] == dep) { done = true; break; }
        if (done) continue;
        if (w->n == WRITTEN_CAP) {
            fprintf(stderr, "cgen: error: imported modules exceed the writing limit %d [implementation-limit]\n", WRITTEN_CAP);
            return 2;
        }
        w->seen[w->n++] = dep;
        char src_buf[8192];
        keel_slice_char src_path = source_path(dep->name, src_buf, sizeof src_buf);
        int rc = emit_one(arena, dest, dep->ast, dep->name, src_path, K_EMIT_TYPE_H, ".type.h", opt);
        if (!rc) rc = emit_one(arena, dest, dep->ast, dep->name, src_path, K_EMIT_H, ".h", opt);
        if (!rc) rc = emit_imports(arena, tool, dest, dep, w, opt, base);
        if (rc) return rc;
    }
    return 0;
}

/* The headers of each instance the module's closure holds; the base's (`keel_*`)
 * only when `base` is set. The invocation writes the headers of everything it
 * reaches (cgen design §1), the base's included: an instance of the base over a
 * type of the program (`keel_routine_ag2_Ag`) has no other place to come from.
 * The generic's .k path is its module name, as for any module. */
/* The instances of a function over a protocol the closure holds (backend
 * §5.19): one file pair per concrete type, `<prefix>_<name>_<concrete>`. The
 * concrete type is the closure's instance whose symbol ends the entry's. */
static int emit_protocol_functions(keel_arena *arena, const char *dest, const KAst *ast, KEmitOptions opt, bool base) {
    for (size_t i = 0; i < ast->closure.len; i++) {
        const KClosure *c = keel_buffer_KClosure_ptr(&ast->closure, i);
        if (c->kind != K_CLOSURE_PROTOCOL_FUNCTION || !c->module || !c->module->ast) continue;
        keel_slice_char gen = c->module->name;
        if (!base && gen.len >= 4 && memcmp(gen.ptr, "keel", 4) == 0 && (gen.len == 4 || gen.ptr[4] == '.')) continue;
        char pre[4096];
        size_t pl = k_mangle_module(gen, (keel_slice_char){ sizeof pre, pre });
        if (pl >= sizeof pre || pl + 1 >= c->symbol_len || memcmp(c->symbol, pre, pl) != 0 || c->symbol[pl] != '_') continue;
        /* <name>_<concrete>: the concrete type is an instance of the closure */
        const KClosure *k = NULL;
        size_t name_len = 0;
        /* the longest: `keel_array_keel_routine_slot_ag2_Ag` ends the symbol, and so does the
           slot's own `keel_routine_slot_ag2_Ag` inside it */
        for (size_t q = 0; q < ast->closure.len; q++) {
            const KClosure *o = keel_buffer_KClosure_ptr(&ast->closure, q);
            if (o->kind != K_CLOSURE_INSTANCE || !o->module || o->symbol_len + pl + 2 >= c->symbol_len) continue;
            if (k && o->symbol_len <= k->symbol_len) continue;
            size_t at = c->symbol_len - o->symbol_len;
            if (memcmp(c->symbol + at, o->symbol, o->symbol_len) != 0 || c->symbol[at - 1] != '_') continue;
            k = o;
            name_len = at - 1 - (pl + 1);
        }
        if (!k) {
            fprintf(stderr, "cgen: error: %.*s: the emitter does not cover this function over a protocol yet [implementation-limit]\n",
                    (int)c->symbol_len, c->symbol);
            return 2;
        }
        KEmitInstance concrete;
        if (!k_emit_instance_of(ast, k, &concrete)) return 2;
        KEmitInstance inst = { .symbol = { c->symbol_len, (char *)c->symbol }, .module = gen,
                               .proto_fn = { name_len, (char *)c->symbol + pl + 1 }, .proto_concrete = concrete.symbol,
                               .proto_module = k->module->name };
        inst.proto_array = k->module->name.len == 10 && memcmp(k->module->name.ptr, "keel.array", 10) == 0;
        if (inst.proto_array && concrete.argc) inst.proto_elem = concrete.arg[0];
        for (size_t q = 0; q < k->module->ast->nodes.len; q++) {
            const KAstNode *m = keel_buffer_KAstNode_ptr(&k->module->ast->nodes, q);
            if (m->kind != K_AST_MODIFIER) continue;
            for (size_t j = m->name_first; j < m->end; j++) {
                KToken t = keel_buffer_KLexeme_ptr(&k->module->ast->tokens, j)->token;
                if (t.len == 5 && memcmp(t.ptr, "byref", 5) == 0) inst.proto_byref = true;
            }
        }
        KAst *generic = (KAst *)c->module->ast;
        KIslandInstance ii = { .self = k->module, .symbol = concrete.symbol, .modifier = concrete.modifier, .argc = concrete.argc,
                               .proto_fn = inst.proto_fn };
        for (size_t q = 0; q < concrete.argc && q < 8; q++) ii.arg[q] = concrete.arg[q];
        KDiagnostic scratch_items[256];
        KDiagnosticSink scratch;
        k_diag_init(&scratch, scratch_items, 256);
        k_collect_instance_islands(generic, &ii, &scratch);
        char src_buf[8192];
        keel_slice_char src_path = source_path(gen, src_buf, sizeof src_buf);
        int rc = 0;
        for (int w = 0; w < 2 && !rc; w++) {
            KEmitFile which = w == 0 ? K_EMIT_TYPE_H : K_EMIT_H;
            const char *ext = w == 0 ? ".type.h" : ".h";
            size_t need = k_emit_instance(generic, &inst, src_path, which, opt, (keel_slice_char){0});
            if (need == K_EMIT_UNSUPPORTED) {
                fprintf(stderr, "cgen: error: %.*s: the emitter does not cover this function over a protocol yet [implementation-limit]\n",
                        (int)c->symbol_len, c->symbol);
                rc = 2;
                break;
            }
            char *data = cgen_alloc(arena, need ? need : 1, 1, 1, false, "generated file");
            if (!data) { rc = 2; break; }
            k_emit_instance(generic, &inst, src_path, which, opt, (keel_slice_char){ need, data });
            char rel[4096], path[8192];
            size_t rel_len = k_mangle_instance_path(gen, k_emit_unit(&inst), ext, (keel_slice_char){ sizeof rel, rel });
            if (rel_len >= sizeof rel) { rc = write_failed(dest); break; }
            int n = snprintf(path, sizeof path, "%s/%.*s", dest, (int)rel_len, rel);
            if (n < 0 || (size_t)n >= sizeof path || !make_parents(path)) { rc = write_failed(dest); break; }
            rc = cgen_write_output(path, data, need);
        }
        generic->islands.len = 0;
        generic->island_text.len = 0;
        generic->closure.len = 0;
        generic->closure_text.len = 0;
        if (rc) return rc;
    }
    return 0;
}

static int emit_instances(keel_arena *arena, const char *dest, const KAst *ast, KEmitOptions opt, bool base) {
    int prc = emit_protocol_functions(arena, dest, ast, opt, base);
    if (prc) return prc;
    for (size_t i = 0; i < ast->closure.len; i++) {
        const KClosure *c = keel_buffer_KClosure_ptr(&ast->closure, i);
        if (c->kind != K_CLOSURE_INSTANCE || !c->module || !c->module->ast) continue;
        keel_slice_char gen = c->module->name;
        if (!base && gen.len >= 4 && memcmp(gen.ptr, "keel", 4) == 0 && (gen.len == 4 || gen.ptr[4] == '.')) continue;
        KEmitInstance inst;
        if (!k_emit_instance_of(ast, c, &inst)) {
            fprintf(stderr, "cgen: error: %.*s: the emitter does not cover this instance yet [implementation-limit]\n",
                    (int)c->symbol_len, c->symbol);
            return 2;
        }
        /* the instance's islands, in the generic's tree, for this instance only */
        KAst *generic = (KAst *)c->module->ast;
        KIslandInstance ii = { .self = c->module, .symbol = inst.symbol, .modifier = inst.modifier, .argc = inst.argc };
        for (size_t k = 0; k < inst.argc && k < 8; k++) ii.arg[k] = inst.arg[k];
        KDiagnostic scratch_items[256];
        KDiagnosticSink scratch;
        k_diag_init(&scratch, scratch_items, 256);
        k_collect_instance_islands(generic, &ii, &scratch);
        char src_buf[8192];
        keel_slice_char src_path = source_path(gen, src_buf, sizeof src_buf);
        for (int w = 0; w < 2; w++) {
            KEmitFile which = w == 0 ? K_EMIT_TYPE_H : K_EMIT_H;
            const char *ext = w == 0 ? ".type.h" : ".h";
            size_t need = k_emit_instance(c->module->ast, &inst, src_path, which, opt, (keel_slice_char){0});
            if (need == K_EMIT_UNSUPPORTED) {
                fprintf(stderr, "cgen: error: %.*s: the emitter does not cover this instance yet [implementation-limit]\n",
                        (int)inst.symbol.len, inst.symbol.ptr);
                return 2;
            }
            char *data = cgen_alloc(arena, need ? need : 1, 1, 1, false, "generated file");
            if (!data) return 2;
            k_emit_instance(c->module->ast, &inst, src_path, which, opt, (keel_slice_char){ need, data });
            char rel[4096], path[8192];
            size_t rel_len = k_mangle_instance_path(gen, k_emit_unit(&inst), ext, (keel_slice_char){ sizeof rel, rel });
            if (rel_len >= sizeof rel) return write_failed(dest);
            int n = snprintf(path, sizeof path, "%s/%.*s", dest, (int)rel_len, rel);
            if (n < 0 || (size_t)n >= sizeof path || !make_parents(path)) return write_failed(dest);
            int rc = cgen_write_output(path, data, need);
            if (rc) return rc;
        }
        generic->islands.len = 0;
        generic->island_text.len = 0;
        generic->closure.len = 0;
        generic->closure_text.len = 0;
    }
    return 0;
}

static bool has_public_main(const KAst *ast) {
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->kind != K_AST_FUNCTION || !n->is_public) continue;
        KToken t = keel_buffer_KLexeme_ptr(&ast->tokens, n->name_first)->token;
        if (t.len == 4 && memcmp(t.ptr, "main", 4) == 0) return true;
    }
    return false;
}

int cgen_stop_after_gen(keel_arena *arena, const char *path, const char *const *roots, int n,
                        const char *base, const char *dest_dir, const char *profile,
                        const char *const *pass, int pass_n, bool line_on, const char *main_module,
                        const char *lowering, CgenUnits *units) {
    if (n < 0 || n > CGEN_CLI_ROOTS_CAP) {
        fprintf(stderr, "cgen: error: search roots exceed configured limit %d; adjust CGEN_CLI_ROOTS_CAP [implementation-limit]\n", CGEN_CLI_ROOTS_CAP);
        return 2;
    }
    char source[8192], normalized[8192], name[8192];
    if (!absolute(path, source, sizeof source)) return 2;
    bool found = false;
    for (int i = 0; i < n; i++)
        if (absolute(roots[i], normalized, sizeof normalized) &&
            cgen_module_name_of(source, normalized, name, sizeof name)) { found = true; break; }
    if (!found) name[0] = '\0';
    const char *search[CGEN_SEARCH_ROOTS_CAP];
    for (int i = 0; i < n; i++) search[i] = roots[i];
    search[n++] = base;
    KDiagnostic items[4096];
    KDiagnosticSink sink;
    k_diag_init(&sink, items, 4096);
    void *tool = cgen_alloc(arena, 1, cgen_loader_size(), _Alignof(max_align_t), true, "loader");
    if (!tool) return 2;
    KLoader loader;
    cgen_loader_init(tool, arena, search, n, &sink, &loader);
    KModule *module = NULL;
    KLoadResult result = cgen_load_path(tool, path, name, &module);
    cgen_loader_report(tool, &sink);
    int rc = 1;
    if (result == K_LOAD_NOT_FOUND) { fprintf(stderr, "cgen: error: cannot read source '%s' [source-not-found]\n", path); rc = 2; }
    if (result == K_LOAD_OK && module && k_diag_count(&sink, K_ERROR) == 0) {
        KModule *prelude = NULL;
        KLoadResult pr = cgen_load(tool, k_diag_text("keel"), &prelude);
        const char *dest = dest_dir ? dest_dir : "gen";
        KEmitOptions opt = { .c23 = profile_is_c23(profile, pass, pass_n), .line_on = line_on,
                          .omp = parallel_is_omp(lowering, pass, pass_n) };
        keel_slice_char mod = k_diag_text(name);
        char src_buf[8192];
        keel_slice_char src_path = source_path(mod, src_buf, sizeof src_buf);
        if (k_emit_module_is_generic(module->ast)) {
            /* a generic module is not translatable alone (cgen design §2.8, §8.8) */
            fprintf(stderr, "cgen: error: %s is a generic module; use --instance [generic-source-without-instance]\n", name);
            rc = 2;
        } else if (main_module && strcmp(main_module, name) != 0) {
            fprintf(stderr, "cgen: error: --main names '%s', but the compiled module is '%s' [invalid-option]\n", main_module, name);
            rc = 2;
        } else if (main_module && !has_public_main(module->ast)) {
            fprintf(stderr, "%s: error: module '%s' has no public function main [missing-entry-point]\n", path, name);
            rc = 2;
        } else if (pr != K_LOAD_OK && pr != K_LOAD_ALREADY) {
            fprintf(stderr, "cgen: error: cannot load the prelude keel [module-not-found]\n");
            rc = 2;
        } else {
            rc = 0;
            keel_slice_char keel_mod = k_diag_text("keel"), keel_src = k_diag_text("keel.k");
            if (!rc) rc = emit_one(arena, dest, prelude->ast, keel_mod, keel_src, K_EMIT_TYPE_H, ".type.h", opt);
            if (!rc) rc = emit_one(arena, dest, prelude->ast, keel_mod, keel_src, K_EMIT_H, ".h", opt);
            if (!rc) rc = emit_instances(arena, dest, module->ast, opt, true);
            if (!rc) rc = emit_one(arena, dest, module->ast, mod, src_path, K_EMIT_TYPE_H, ".type.h", opt);
            if (!rc) rc = emit_one(arena, dest, module->ast, mod, src_path, K_EMIT_H, ".h", opt);
            if (!rc) rc = emit_one(arena, dest, module->ast, mod, src_path, K_EMIT_C, ".c", opt);
            Written written = { .n = 0 };
            written.seen[written.n++] = prelude;
            written.seen[written.n++] = module;
            if (!rc) rc = emit_imports(arena, tool, dest, module, &written, opt, true);
            if (!rc && main_module) rc = emit_one(arena, dest, NULL, mod, src_path, K_EMIT_ENTRY, NULL, opt);
            if (!rc && units) {
                /* the translation units the C compiler is given (cgen-tool-spec §4.3, §4.7) */
                char rel[4096];
                size_t rl = k_mangle_path(mod, ".c", (keel_slice_char){ sizeof rel, rel });
                if (rl >= sizeof rel || snprintf(units->unit, sizeof units->unit, "%s/%.*s", dest, (int)rl, rel) >= (int)sizeof units->unit) rc = write_failed(dest);
                units->entry[0] = '\0';
                units->source_count = cgen_loader_sources(tool, units->sources, CGEN_SOURCES_CAP);
                if (!rc && main_module) {
                    rl = k_mangle_entry(mod, (keel_slice_char){ sizeof rel, rel });
                    if (rl >= sizeof rel || snprintf(units->entry, sizeof units->entry, "%s/%.*s", dest, (int)rl, rel) >= (int)sizeof units->entry) rc = write_failed(dest);
                }
            }
        }
    }
    cgen_loader_destroy(tool);
    return rc;
}

/* ---- --instance (cgen design §2.8, §8.7) ----
 * The use is written as a file-scope `instance` would write it, every name
 * qualified in full. It becomes a module of its own, in memory, that imports
 * each module the use qualifies and places the instance: the same closure, the
 * same emission. Written: the prelude's headers, the headers of every module
 * and instance the closure reaches (the base's too: this is how the base is
 * built), and the instance's .c, with the bodies of its non-inline functions. */

#define USE_CAP 4096

/* appends s[0..n) to buf at *w; false when it does not fit */
static bool put(char *buf, size_t *w, const char *s, size_t n) {
    if (*w + n >= USE_CAP) return false;
    memcpy(buf + *w, s, n);
    *w += n;
    buf[*w] = '\0';
    return true;
}

static bool ident_char(char c) { return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }

int cgen_stop_after_instance(keel_arena *arena, const char *use, const char *const *roots, int n,
                             const char *base, const char *dest_dir, const char *profile,
                             const char *const *pass, int pass_n, bool line_on, const char *lowering, CgenUnits *units) {
    if (n < 0 || n > CGEN_CLI_ROOTS_CAP) {
        fprintf(stderr, "cgen: error: search roots exceed configured limit %d; adjust CGEN_CLI_ROOTS_CAP [implementation-limit]\n", CGEN_CLI_ROOTS_CAP);
        return 2;
    }
    const char *search[CGEN_SEARCH_ROOTS_CAP];
    for (int i = 0; i < n; i++) search[i] = roots[i];
    search[n++] = base;
    KDiagnostic items[4096];
    KDiagnosticSink sink;
    k_diag_init(&sink, items, 4096);
    void *tool = cgen_alloc(arena, 1, cgen_loader_size(), _Alignof(max_align_t), true, "loader");
    if (!tool) return 2;
    KLoader loader;
    cgen_loader_init(tool, arena, search, n, &sink, &loader);

    /* every dotted name of the use: its module is imported under an alias of
       its own, and the name is written through it */
    char imports[USE_CAP] = "", body[USE_CAP] = "";
    size_t iw = 0, bw = 0, aliases = 0;
    const char *generic_name = NULL;
    size_t generic_len = 0;
    /* a generic without a modifier, named whole (`keel.array i32`): its instance
       is the family of verbs of the arguments, reached by a call of one of them
       over an `array` of the arguments (backend §5.2, rule 8) */
    const char *start = use;
    while (*start == ' ') start++;
    const char *wend = start;
    while (*wend && (ident_char(*wend) || *wend == '.')) wend++;
    KModule *family = NULL;
    keel_slice_char verb = {0};
    if (memchr(start, '.', (size_t)(wend - start))) {
        KModule *m = NULL;
        KLoadResult r = cgen_load(tool, (keel_slice_char){ (size_t)(wend - start), (char *)start }, &m);
        if ((r == K_LOAD_OK || r == K_LOAD_ALREADY) && m && m->ast && k_emit_generic_without_modifier(m->ast, &verb)) {
            family = m;
            generic_name = start;
            generic_len = (size_t)(wend - start);
            if (!put(imports, &iw, "import ", 7) || !put(imports, &iw, start, generic_len) ||
                !put(imports, &iw, " as cgen_m0;\n", 13)) return 2;
            aliases = 1;
        }
    }
    /* a function over a protocol, then the concrete type (`keel.slice.of
       keel.buffer.buffer i32`, backend §5.19): reached by a call of it over a
       variable of that type */
    keel_slice_char proto_fn = {0};
    bool proto_array = false;
    const char *concrete = wend;
    if (!family) {
        const char *dot = NULL;
        for (const char *q = start; q < wend; q++) if (*q == '.') dot = q;
        KModule *m = NULL;
        if (dot && cgen_load(tool, (keel_slice_char){ (size_t)(dot - start), (char *)start }, &m) != K_LOAD_NOT_FOUND && m && m->ast &&
            k_emit_protocol_function(m->ast, (keel_slice_char){ (size_t)(wend - dot - 1), (char *)dot + 1 })) {
            proto_fn = (keel_slice_char){ (size_t)(wend - dot - 1), (char *)dot + 1 };
            generic_name = start;
            generic_len = (size_t)(dot - start);
            if (!put(imports, &iw, "import ", 7) || !put(imports, &iw, start, generic_len) ||
                !put(imports, &iw, " as cgen_mp;\n", 13)) return 2;
            /* the concrete type: `keel.array T` is an `array` of T */
            while (*concrete == ' ') concrete++;
            const char *ce = concrete;
            while (*ce && (ident_char(*ce) || *ce == '.')) ce++;
            KModule *cm = NULL;
            keel_slice_char unused;
            if (memchr(concrete, '.', (size_t)(ce - concrete)) &&
                cgen_load(tool, (keel_slice_char){ (size_t)(ce - concrete), (char *)concrete }, &cm) != K_LOAD_NOT_FOUND &&
                cm && cm->ast && k_emit_generic_without_modifier(cm->ast, &unused)) {
                proto_array = true;
                if (!put(imports, &iw, "import ", 7) || !put(imports, &iw, concrete, (size_t)(ce - concrete)) ||
                    !put(imports, &iw, " as cgen_ma;\n", 13)) return 2;
                concrete = ce;
            }
        }
    }
    /* §2.8, step 3: the first name is <module>.<modifier> of a generic module */
    if (!family && !proto_fn.len) {
        const char *dot = NULL;
        for (const char *q = start; q < wend; q++) if (*q == '.') dot = q;
        int wl = (int)(wend - start);
        if (!dot) {
            fprintf(stderr, "cgen: error: '%.*s' does not name a modifier in full [invalid-instance]\n", wl, start);
            cgen_loader_destroy(tool);
            return 2;
        }
        KModule *m = NULL;
        KLoadResult r = cgen_load(tool, (keel_slice_char){ (size_t)(dot - start), (char *)start }, &m);
        if (r == K_LOAD_NOT_FOUND) {
            fprintf(stderr, "cgen: error: module '%.*s' not found [module-not-found]\n", (int)(dot - start), start);
            cgen_loader_destroy(tool);
            return 2;
        }
        if ((r != K_LOAD_OK && r != K_LOAD_ALREADY) || !m || !m->ast) { cgen_loader_report(tool, &sink); cgen_loader_destroy(tool); return 1; }
        bool declares = false;
        keel_slice_char mod = { (size_t)(wend - dot - 1), (char *)dot + 1 };
        for (size_t k = 0; k < m->symbol_count; k++)
            if (m->symbols[k].kind == K_SYM_MODIFIER && m->symbols[k].name.len == mod.len &&
                memcmp(m->symbols[k].name.ptr, mod.ptr, mod.len) == 0) declares = true;
        if (!k_emit_module_is_generic(m->ast) || !declares) {
            fprintf(stderr, "cgen: error: '%.*s' does not name a modifier in full; module '%.*s' %s [invalid-instance]\n",
                    wl, start, (int)(dot - start), start,
                    !k_emit_module_is_generic(m->ast) ? "is not generic" : "declares no such modifier");
            cgen_loader_destroy(tool);
            return 2;
        }
    }
    for (const char *p = family ? wend : proto_fn.len ? concrete : use; *p; ) {
        if (!ident_char(*p)) { if (!put(body, &bw, p, 1)) return 2; p++; continue; }
        const char *q = p, *last_dot = NULL;
        while (*q && (ident_char(*q) || (*q == '.' && ident_char(q[1])))) { if (*q == '.') last_dot = q; q++; }
        if (!last_dot) { if (!put(body, &bw, p, (size_t)(q - p))) return 2; p = q; continue; }
        size_t ml = (size_t)(last_dot - p);
        KModule *m = NULL;
        KLoadResult r = cgen_load(tool, (keel_slice_char){ ml, (char *)p }, &m);
        if (r == K_LOAD_NOT_FOUND) {
            fprintf(stderr, "cgen: error: module '%.*s' not found [module-not-found]\n", (int)ml, p);
            cgen_loader_destroy(tool);
            return 2;
        }
        if ((r != K_LOAD_OK && r != K_LOAD_ALREADY) || !m) { cgen_loader_report(tool, &sink); cgen_loader_destroy(tool); return 1; }
        if (!generic_name) { generic_name = p; generic_len = ml; }
        char alias[32];
        int al = snprintf(alias, sizeof alias, "cgen_m%zu", aliases++);
        if (!put(imports, &iw, "import ", 7) || !put(imports, &iw, p, ml) || !put(imports, &iw, " as ", 4) ||
            !put(imports, &iw, alias, (size_t)al) || !put(imports, &iw, ";\n", 2) ||
            !put(body, &bw, alias, (size_t)al) || !put(body, &bw, last_dot, (size_t)(q - last_dot))) return 2;
        p = q;
    }
    if (!generic_name) {
        fprintf(stderr, "cgen: error: '%s' does not name a modifier in full [invalid-instance]\n", use);
        cgen_loader_destroy(tool);
        return 2;
    }
    char text[3 * USE_CAP];
    int tl = proto_fn.len
        ? snprintf(text, sizeof text, "module cgen_instance;\n%spriv %s%s cgen_x%s;\npriv void cgen_use(void) { (void)cgen_mp.%.*s(cgen_x); }\n",
                   imports, proto_array ? "array " : "", body, proto_array ? "[1]" : "", (int)proto_fn.len, proto_fn.ptr)
        : family
        ? snprintf(text, sizeof text, "module cgen_instance;\n%spriv array%s cgen_v[1];\npriv void cgen_use(void) { (void)cgen_m0.%.*s(cgen_v); }\n",
                   imports, body, (int)verb.len, verb.ptr)
        : snprintf(text, sizeof text, "module cgen_instance;\n%sinstance %s;\n", imports, body);
    if (tl < 0 || (size_t)tl >= sizeof text) return 2;
    char *saved = cgen_alloc(arena, (size_t)tl + 1, 1, 1, false, "instance source");
    if (!saved) return 2;
    memcpy(saved, text, (size_t)tl + 1);

    KModule *module = NULL;
    KLoadResult result = cgen_load_text(tool, "<instance>", "cgen_instance", (keel_slice_char){ (size_t)tl, saved }, &module);
    /* the synthetic module's own redundant-instance would name its aliases: the
       invocation reports it below, in the words of the use */
    size_t kept = 0;
    bool mismatch = false;
    for (size_t i = 0; i < sink.len; i++) {
        if (sink.items[i].id == K_DIAG_INSTANCE_NOT_MODIFIER) mismatch = true;
        else if (sink.items[i].id != K_DIAG_REDUNDANT_INSTANCE) sink.items[kept++] = sink.items[i];
    }
    sink.len = kept;
    if (mismatch) {
        /* §2.8, step 4: the arguments do not match the `module` line */
        fprintf(stderr, "cgen: error: '%s': the arguments do not match the parameters of the generic [invalid-instance]\n", use);
        cgen_loader_destroy(tool);
        return 2;
    }
    cgen_loader_report(tool, &sink);
    int rc = 1;
    if (result != K_LOAD_OK || !module || k_diag_count(&sink, K_ERROR) != 0) { cgen_loader_destroy(tool); return rc; }

    /* the instance the use names: the entry `instance` wrote */
    const KAst *ast = module->ast;
    const KClosure *target = NULL;
    for (size_t i = 0; i < ast->closure.len && !target; i++) {
        const KClosure *c = keel_buffer_KClosure_ptr(&ast->closure, i);
        if (proto_fn.len ? c->kind == K_CLOSURE_PROTOCOL_FUNCTION
                         : c->kind == K_CLOSURE_INSTANCE && (family ? c->module == family : c->origin == K_CLOSURE_WRITTEN)) target = c;
    }
    if (!target || !target->module) {
        fprintf(stderr, "cgen: error: '%s' does not name a modifier in full [invalid-instance]\n", use);
        cgen_loader_destroy(tool);
        return 2;
    }
    KModule *prelude = NULL;
    KLoadResult pr = cgen_load(tool, k_diag_text("keel"), &prelude);
    if (pr != K_LOAD_OK && pr != K_LOAD_ALREADY) {
        fprintf(stderr, "cgen: error: cannot load the prelude keel [module-not-found]\n");
        cgen_loader_destroy(tool);
        return 2;
    }
    const char *dest = dest_dir ? dest_dir : "gen";
    KEmitOptions opt = { .c23 = profile_is_c23(profile, pass, pass_n), .line_on = line_on,
                          .omp = parallel_is_omp(lowering, pass, pass_n) };
    keel_slice_char keel_mod = k_diag_text("keel"), keel_src = k_diag_text("keel.k");
    rc = emit_one(arena, dest, prelude->ast, keel_mod, keel_src, K_EMIT_TYPE_H, ".type.h", opt);
    if (!rc) rc = emit_one(arena, dest, prelude->ast, keel_mod, keel_src, K_EMIT_H, ".h", opt);
    if (!rc) rc = emit_instances(arena, dest, ast, opt, true);
    Written written = { .n = 0 };
    written.seen[written.n++] = prelude;
    written.seen[written.n++] = module;
    if (!rc) rc = emit_imports(arena, tool, dest, module, &written, opt, true);

    /* the instance's .c: its header, then the bodies of its non-inline functions;
       a function over a protocol is `inline` in its own headers, and has none */
    if (!rc && !proto_fn.len) {
        KEmitInstance inst;
        if (!k_emit_instance_of(ast, target, &inst)) rc = 2;
        char src_buf[8192];
        keel_slice_char src_path = source_path(target->module->name, src_buf, sizeof src_buf);
        size_t need = rc ? 0 : k_emit_instance(target->module->ast, &inst, src_path, K_EMIT_C, opt, (keel_slice_char){0});
        if (!rc && need == K_EMIT_UNSUPPORTED) {
            fprintf(stderr, "cgen: error: %.*s: the emitter does not cover this instance yet [implementation-limit]\n",
                    (int)inst.symbol.len, inst.symbol.ptr);
            rc = 2;
        }
        if (!rc) {
            char *data = cgen_alloc(arena, need ? need : 1, 1, 1, false, "generated file");
            if (!data) rc = 2;
            else {
                k_emit_instance(target->module->ast, &inst, src_path, K_EMIT_C, opt, (keel_slice_char){ need, data });
                if (k_emit_instance_redundant(target->module->ast))
                    fprintf(stderr, "cgen: warning: %.*s%s%.*s is entirely pub inline; the .c will have no body [redundant-instance]\n",
                            (int)target->module->name.len, target->module->name.ptr, inst.modifier.len ? "." : "",
                            (int)inst.modifier.len, inst.modifier.ptr);
                char rel[4096], path[8192];
                size_t rel_len = k_mangle_instance_path(target->module->name, k_emit_unit(&inst), ".c", (keel_slice_char){ sizeof rel, rel });
                int w = rel_len < sizeof rel ? snprintf(path, sizeof path, "%s/%.*s", dest, (int)rel_len, rel) : -1;
                if (w < 0 || (size_t)w >= sizeof path || !make_parents(path)) rc = write_failed(dest);
                else rc = cgen_write_output(path, data, need);
                if (!rc && units && snprintf(units->unit, sizeof units->unit, "%s", path) >= (int)sizeof units->unit) rc = write_failed(dest);
            }
        }
    }
    (void)generic_len;
    if (!rc && units) units->source_count = cgen_loader_sources(tool, units->sources, CGEN_SOURCES_CAP);
    cgen_loader_destroy(tool);
    return rc;
}
