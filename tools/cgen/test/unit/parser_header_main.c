/* Minimal oracle for the m2-parser-header task — one happy-path case per
 * function, plus the `next_out` check (the exact contract that broke the
 * first version of this task, 2026-09-26). Not model-generated. */

#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && memcmp((s).ptr, want, (s).len) == 0)

static KToken first_token(const char *src, KLexer *lexer) {
    keel_slice_char source = { strlen(src), (char *)src };
    TKPpKind pp;
    k_lexer_init(lexer, source, NULL);
    return k_lexer_next(lexer, &pp);
}

static void check_next(const char *label, KToken next) {
    if (!EQ(next, "ok")) {
        fprintf(stderr, "FAIL: %s — next_out = \"%.*s\", want \"ok\"\n", label, (int)next.len, next.ptr);
        failures++;
    }
}

int main(void) {
    KLexer lexer;
    TKPpKind pp;
    KToken next;

    /* module app.cfg; ok */
    KToken kw = first_token("module app.cfg; ok\n", &lexer); /* kw = the 'module' token itself */
    KModuleHeader mh;
    k_scan_module_decl(&lexer, kw, &mh, &next, &pp);
    if (!EQ(mh.module_name, "app.cfg")) { fprintf(stderr, "FAIL: module_name = \"%.*s\"\n", (int)mh.module_name.len, mh.module_name.ptr); failures++; }
    check_next("module", next);

    /* module coll dim N tags Cycle type T; ok -- all three binders */
    KToken kw2 = first_token("module coll dim N tags Cycle type T; ok\n", &lexer);
    KModuleHeader mh2;
    k_scan_module_decl(&lexer, kw2, &mh2, &next, &pp);
    if (mh2.dim_count != 1 || !EQ(mh2.dims[0], "N")) { fprintf(stderr, "FAIL: dims\n"); failures++; }
    if (mh2.tag_count != 1 || !EQ(mh2.tags[0], "Cycle")) { fprintf(stderr, "FAIL: tags\n"); failures++; }
    if (mh2.type_count != 1 || !EQ(mh2.types[0], "T")) { fprintf(stderr, "FAIL: types\n"); failures++; }
    check_next("module with binders", next);

    /* import keel.buffer as buffer types; ok */
    KToken ikw = first_token("import keel.buffer as buffer types; ok\n", &lexer);
    KImportDecl imp;
    k_scan_import(&lexer, ikw, &imp, &next, &pp);
    if (!EQ(imp.module_name, "keel.buffer")) { fprintf(stderr, "FAIL: import module_name = \"%.*s\"\n", (int)imp.module_name.len, imp.module_name.ptr); failures++; }
    if (!EQ(imp.alias, "buffer")) { fprintf(stderr, "FAIL: import alias = \"%.*s\"\n", (int)imp.alias.len, imp.alias.ptr); failures++; }
    if (!imp.has_types) { fprintf(stderr, "FAIL: import has_types = false\n"); failures++; }
    check_next("import", next);

    /* import_c <stdio.h>; ok */
    KToken ckw = first_token("import_c <stdio.h>; ok\n", &lexer);
    KImportCDecl ic;
    k_scan_import_c(&lexer, ckw, &ic, &next, &pp);
    if (!EQ(ic.header, "<stdio.h>")) { fprintf(stderr, "FAIL: import_c header = \"%.*s\"\n", (int)ic.header.len, ic.header.ptr); failures++; }
    check_next("import_c", next);

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
