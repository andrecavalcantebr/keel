/* Oracle for tool/tool.c — the invoker half of the loader (cgen-tool
 * §3.1, §3.3). Expected values hand-derived from the repository's own
 * base/ tree and from keel-spec §4.1. Not model-generated.
 *
 * Unlike every other unit oracle here, this one does open files: the
 * whole point of this half is that it knows what a file is. It reads
 * the repository's real base/, so it needs no fixture tree, and it is
 * run from the repository root.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/ast.h"
#include "engine/loader.h"

void   cgen_loader_init(void *tool, const char *const *roots, int root_count,
                        KDiagnosticSink *diag, KLoader *out);
size_t cgen_loader_size(void);

static int failures = 0;
#define S(lit) ((keel_slice_char){ sizeof(lit) - 1, (char *)(lit) })

static const KSymbol *find_sym(const KModule *m, const char *name) {
    keel_slice_char want = { strlen(name), (char *)name };
    for (size_t i = 0; i < m->symbol_count; i++)
        if (k_symtab_same_name(m->symbols[i].name, want)) return &m->symbols[i];
    return NULL;
}

int main(void) {
    const char *roots[] = { "base" };
    KDiagnostic diags[64];
    KDiagnosticSink sink;
    k_diag_init(&sink, diags, 64);

    void *tool = calloc(1, cgen_loader_size());
    KLoader loader;
    cgen_loader_init(tool, roots, 1, &sink, &loader);

    /* the prelude: base/keel.k, found under the one root */
    KModule *keel = NULL;
    KLoadResult r = loader.load(loader.tool, S("keel"), &keel);
    if (r != K_LOAD_OK || keel == NULL) {
        fprintf(stderr, "FAIL: keel — result %d, module %p\n", (int)r, (void *)keel);
        failures++;
    } else {
        /* base/keel.k declares the primitive typedefs */
        const char *want[] = { "i8", "u8", "i32", "u32", "f32", "f64" };
        for (size_t i = 0; i < sizeof want / sizeof *want; i++) {
            const KSymbol *s = find_sym(keel, want[i]);
            if (s == NULL) {
                fprintf(stderr, "FAIL: keel exports no `%s`\n", want[i]);
                failures++;
            } else if (s->kind != K_SYM_TYPE) {
                fprintf(stderr, "FAIL: keel's `%s` is kind %d, want K_SYM_TYPE\n",
                        want[i], (int)s->kind);
                failures++;
            }
        }
        if (keel->closure_mtime <= 0) {
            fprintf(stderr, "FAIL: keel — closure_mtime not set\n");
            failures++;
        }
    }

    /* asking again is memoization, not a cycle: the module is DONE */
    KModule *again = NULL;
    r = loader.load(loader.tool, S("keel"), &again);
    if (r != K_LOAD_ALREADY || again != keel) {
        fprintf(stderr, "FAIL: second keel — result %d (want K_LOAD_ALREADY), same=%d\n",
                (int)r, again == keel);
        failures++;
    }

    /* a dotted name resolves to <root>/<a>/<b>.k, and `slice` is a
       modifier there — arity comes from the module's own binders */
    KModule *slice = NULL;
    r = loader.load(loader.tool, S("keel.slice"), &slice);
    if (r != K_LOAD_OK || slice == NULL) {
        fprintf(stderr, "FAIL: keel.slice — result %d\n", (int)r);
        failures++;
    } else {
        const KSymbol *s = find_sym(slice, "slice");
        if (s == NULL || s->kind != K_SYM_MODIFIER) {
            fprintf(stderr, "FAIL: keel.slice exports no `slice` modifier\n");
            failures++;
        } else if (s->arity != 1) {
            fprintf(stderr, "FAIL: `slice` arity %d, want 1\n", s->arity);
            failures++;
        }
    }

    /* no root has it */
    KModule *missing = NULL;
    r = loader.load(loader.tool, S("no.such.module"), &missing);
    if (r != K_LOAD_NOT_FOUND || missing != NULL) {
        fprintf(stderr, "FAIL: missing — result %d, want K_LOAD_NOT_FOUND\n", (int)r);
        failures++;
    }

    /* the whole base loads, every module, with no lexical diagnostic */
    static const char *const every[] = {
        "keel", "keel.arena", "keel.slice", "keel.buffer", "keel.outcome",
        "keel.range", "keel.corot", "keel.tagged", "keel.array", "keel.routine",
    };
    for (size_t i = 0; i < sizeof every / sizeof *every; i++) {
        KModule *m = NULL;
        keel_slice_char name = { strlen(every[i]), (char *)every[i] };
        KLoadResult rr = loader.load(loader.tool, name, &m);
        if ((rr != K_LOAD_OK && rr != K_LOAD_ALREADY) || m == NULL) {
            fprintf(stderr, "FAIL: %s — result %d\n", every[i], (int)rr);
            failures++;
        }
    }
    if (k_diag_count(&sink, K_ERROR) != 0) {
        fprintf(stderr, "FAIL: the base produced %zu errors while loading\n",
                k_diag_count(&sink, K_ERROR));
        failures++;
    }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
