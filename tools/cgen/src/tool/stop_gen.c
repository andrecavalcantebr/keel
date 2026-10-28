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
                        Written *w, KEmitOptions opt) {
    const KAst *ast = m->ast;
    for (size_t i = 0; i < ast->nodes.len; i++) {
        const KAstNode *n = keel_buffer_KAstNode_ptr(&ast->nodes, i);
        if (n->kind != K_AST_IMPORT) continue;
        KModule *dep = NULL;
        KLoadResult r = cgen_load(tool, name_span(ast, n->name_first, n->name_end), &dep);
        if ((r != K_LOAD_OK && r != K_LOAD_ALREADY) || !dep) return 2;
        /* the base's modules (keel.*) are written with the base, not by their users */
        keel_slice_char dn = dep->name;
        if (dn.len > 5 && memcmp(dn.ptr, "keel.", 5) == 0) continue;
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
        if (!rc) rc = emit_imports(arena, tool, dest, dep, w, opt);
        if (rc) return rc;
    }
    return 0;
}

/* The headers of each instance the module's closure holds, of a generic module
 * that is not the base (the base's `keel_*` instances come from the transform,
 * in gen/). The generic's .k path is its module name, as for any module. */
static int emit_instances(keel_arena *arena, const char *dest, const KAst *ast, KEmitOptions opt) {
    for (size_t i = 0; i < ast->closure.len; i++) {
        const KClosure *c = keel_buffer_KClosure_ptr(&ast->closure, i);
        if (c->kind != K_CLOSURE_INSTANCE || !c->module || !c->module->ast) continue;
        keel_slice_char gen = c->module->name;
        if (gen.len >= 4 && memcmp(gen.ptr, "keel", 4) == 0 && (gen.len == 4 || gen.ptr[4] == '.')) continue;
        KEmitInstance inst;
        if (!k_emit_instance_of(ast, c, &inst)) {
            fprintf(stderr, "cgen: error: %.*s: the emitter does not cover this instance yet [implementation-limit]\n",
                    (int)c->symbol_len, c->symbol);
            return 2;
        }
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
            size_t rel_len = k_mangle_instance_path(gen, inst.symbol, ext, (keel_slice_char){ sizeof rel, rel });
            if (rel_len >= sizeof rel) return write_failed(dest);
            int n = snprintf(path, sizeof path, "%s/%.*s", dest, (int)rel_len, rel);
            if (n < 0 || (size_t)n >= sizeof path || !make_parents(path)) return write_failed(dest);
            int rc = cgen_write_output(path, data, need);
            if (rc) return rc;
        }
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
                        const char *const *pass, int pass_n, bool line_on, const char *main_module) {
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
    if (result == K_LOAD_NOT_FOUND) fprintf(stderr, "%s: error: cannot read source [module-not-found]\n", path);
    if (result == K_LOAD_OK && module && k_diag_count(&sink, K_ERROR) == 0) {
        KModule *prelude = NULL;
        KLoadResult pr = cgen_load(tool, k_diag_text("keel"), &prelude);
        const char *dest = dest_dir ? dest_dir : "gen";
        KEmitOptions opt = { .c23 = profile_is_c23(profile, pass, pass_n), .line_on = line_on };
        keel_slice_char mod = k_diag_text(name);
        char src_buf[8192];
        keel_slice_char src_path = source_path(mod, src_buf, sizeof src_buf);
        if (main_module && strcmp(main_module, name) != 0) {
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
            if (!rc) rc = emit_instances(arena, dest, module->ast, opt);
            if (!rc) rc = emit_one(arena, dest, module->ast, mod, src_path, K_EMIT_TYPE_H, ".type.h", opt);
            if (!rc) rc = emit_one(arena, dest, module->ast, mod, src_path, K_EMIT_H, ".h", opt);
            if (!rc) rc = emit_one(arena, dest, module->ast, mod, src_path, K_EMIT_C, ".c", opt);
            Written written = { .n = 0 };
            written.seen[written.n++] = prelude;
            written.seen[written.n++] = module;
            if (!rc) rc = emit_imports(arena, tool, dest, module, &written, opt);
            if (!rc && main_module) rc = emit_one(arena, dest, NULL, mod, src_path, K_EMIT_ENTRY, NULL, opt);
        }
    }
    cgen_loader_destroy(tool);
    return rc;
}
