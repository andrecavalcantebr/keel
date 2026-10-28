/* Oracle of engine/emit/emit.c (M3, tools/agents/tasks/T7-emit.md). Every
 * expected file is hand-written (test/cgen/gen/hello/expected/ from
 * cgen design §8.3; golden/<p>/keel.* from the backend), never captured from
 * the emitter. Run from the repository root. Not model-generated. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/emit/emit.h"

static char *slurp(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "FAIL: cannot open %s\n", path); exit(1); }
    fseek(f, 0, SEEK_END); long len = ftell(f); rewind(f);
    char *s = malloc((size_t)len + 1);
    if (!s || fread(s, 1, (size_t)len, f) != (size_t)len) { fprintf(stderr, "FAIL: read %s\n", path); exit(1); }
    s[len] = 0; fclose(f); *n = (size_t)len;
    return s;
}

static KAst parse(keel_slice_char source) {
    size_t count = k_lexemes(source, NULL, 0, NULL);
    KLexeme *tokens = calloc(count + 1, sizeof *tokens);
    KAstNode *nodes = calloc(count + 1, sizeof *nodes);
    if (!tokens || !nodes) exit(1);
    k_lexemes(source, tokens, count, NULL);
    KAst ast = { .source = source, .tokens = keel_buffer_KLexeme_of(tokens, count),
                 .nodes = keel_buffer_KAstNode_from(nodes, count + 1) };
    if (!k_parse_ast(&ast)) { fprintf(stderr, "FAIL: parse\n"); exit(1); }
    return ast;
}

#define SL(s) ((keel_slice_char){ strlen(s), (char *)(s) })

static int failures = 0;

static void check(const KAst *ast, const char *module, const char *src_path, KEmitFile which,
                  bool c23, const char *expected_path) {
    KEmitOptions opt = { .c23 = c23, .line_on = true };
    size_t want_len; char *want = slurp(expected_path, &want_len);
    size_t need = k_emit_file(ast, SL(module), SL(src_path), which, opt, (keel_slice_char){0});
    if (need == K_EMIT_UNSUPPORTED) { fprintf(stderr, "FAIL: %s: unsupported\n", expected_path); failures++; return; }
    char *buf = malloc(need + 1);
    size_t got = k_emit_file(ast, SL(module), SL(src_path), which, opt, (keel_slice_char){ need, buf });
    if (got != need) { fprintf(stderr, "FAIL: %s: sizing pass %zu != filling pass %zu\n", expected_path, need, got); failures++; }
    if (need != want_len || memcmp(buf, want, want_len) != 0) {
        fprintf(stderr, "FAIL: %s differs\n--- got (%zu bytes) ---\n%.*s\n--- want (%zu bytes) ---\n%s\n",
                expected_path, need, (int)need, buf, want_len, want);
        failures++;
    }
    /* A short buffer: the length is still the full one and nothing is written past it. */
    char small[8]; memset(small, 'Z', sizeof small);
    size_t again = k_emit_file(ast, SL(module), SL(src_path), which, opt, (keel_slice_char){ 4, small });
    if (again != need || small[4] != 'Z') { fprintf(stderr, "FAIL: %s: short buffer\n", expected_path); failures++; }
    free(buf); free(want);
}

int main(void) {
    static const char *prof[2] = { "c23", "c11" };
    size_t n;
    char *hello = slurp("test/cgen/gen/hello/hello.k", &n);
    KAst hello_ast = parse((keel_slice_char){ n, hello });
    char *keel = slurp("base/keel.k", &n);
    KAst keel_ast = parse((keel_slice_char){ n, keel });
    for (int i = 0; i < 2; i++) {
        bool c23 = i == 0;
        char p[160];
        #define EXP(fmt, ...) (snprintf(p, sizeof p, fmt, __VA_ARGS__), p)
        check(&hello_ast, "hello", "hello.k", K_EMIT_TYPE_H, c23, EXP("test/cgen/gen/hello/expected/%s/hello.type.h", prof[i]));
        check(&hello_ast, "hello", "hello.k", K_EMIT_H,      c23, EXP("test/cgen/gen/hello/expected/%s/hello.h", prof[i]));
        check(&hello_ast, "hello", "hello.k", K_EMIT_C,      c23, EXP("test/cgen/gen/hello/expected/%s/hello.c", prof[i]));
        check(NULL,       "hello", "hello.k", K_EMIT_ENTRY,  c23, EXP("test/cgen/gen/hello/expected/%s/main_hello.c", prof[i]));
        check(&keel_ast,  "keel",  "keel.k",  K_EMIT_TYPE_H, c23, EXP("golden/%s/keel.type.h", prof[i]));
        check(&keel_ast,  "keel",  "keel.k",  K_EMIT_H,      c23, EXP("golden/%s/keel.h", prof[i]));
    }
    /* --line=off: the same hello.c with no #line (backend §6). */
    {
        KEmitOptions opt = { .c23 = true, .line_on = false };
        size_t need = k_emit_file(&hello_ast, SL("hello"), SL("hello.k"), K_EMIT_C, opt, (keel_slice_char){0});
        char *buf = malloc(need + 1);
        k_emit_file(&hello_ast, SL("hello"), SL("hello.k"), K_EMIT_C, opt, (keel_slice_char){ need, buf });
        buf[need] = 0;
        if (strstr(buf, "#line") || !strstr(buf, "int hello_main(int argc, char **argv) {")) {
            fprintf(stderr, "FAIL: --line=off still writes #line or lost the function\n"); failures++;
        }
        free(buf);
    }
    /* Outside the covered slice: refused, not mistranslated. */
    {
        char *src = "module m;\ninstance coll.stack i32;\n";
        KAst ast = parse(SL(src));
        KEmitOptions opt = { .c23 = true, .line_on = true };
        if (k_emit_file(&ast, SL("m"), SL("m.k"), K_EMIT_TYPE_H, opt, (keel_slice_char){0}) != K_EMIT_UNSUPPORTED) {
            fprintf(stderr, "FAIL: an `instance` must be K_EMIT_UNSUPPORTED\n"); failures++;
        }
    }
    if (failures) return 1;
    printf("ok     emit: hello (4 files) and the prelude (2 headers), c23 and c11\n");
    return 0;
}
