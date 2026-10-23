/* Oracle of engine/emit/writer.c (KWriter, backend §6). Every expected text
 * is derived by hand from the contract in writer.h. Not model-generated. */
#include <stdio.h>
#include <string.h>
#include "engine/emit/writer.h"

static int failures = 0;
static char buf[4096];

static keel_slice_char S(const char *s) { return (keel_slice_char){ strlen(s), (char *)s }; }

static void start(KWriter *w, bool line_on) {
    memset(buf, '#', sizeof buf);
    k_writer_init(w, (keel_slice_char){ sizeof buf, buf }, S("hello.k"), line_on);
}

static void expect(const char *name, const KWriter *w, const char *want) {
    size_t n = k_writer_len(w);
    if (n != strlen(want) || memcmp(buf, want, n) != 0) {
        fprintf(stderr, "FAIL: %s\n--- want (%zu bytes)\n%s--- got (%zu bytes)\n%.*s---\n",
                name, strlen(want), want, n, (int)(n < sizeof buf ? n : sizeof buf), buf);
        failures++;
    }
}

static void expect_mapped(const char *name, const KWriter *w, size_t want) {
    if (w->mapped != want) {
        fprintf(stderr, "FAIL: %s: mapped = %zu, want %zu\n", name, w->mapped, want);
        failures++;
    }
}

int main(void) {
    KWriter w;

    /* 1. hello.type.h of cgen design §8.3, minus the blank-line question:
     *    raw lines leave mapped at 0; the first k_emit_at(2) writes #line 2,
     *    and mapped = 2 + 1 = 3; the raw #endif advances it to 4. */
    start(&w, true);
    k_emit_raw(&w, S("/* hello.type.h */\n#ifndef HELLO_TYPE_H\n#define HELLO_TYPE_H\n#include \"keel.type.h\"\n"));
    expect_mapped("1 raw before any #line", &w, 0);
    if (!k_emit_at(&w, 2, S("#include <stdio.h>\n"))) { fprintf(stderr, "FAIL: 1 k_emit_at returned false\n"); failures++; }
    expect_mapped("1 after line 2", &w, 3);
    k_emit_raw(&w, S("#endif /* HELLO_TYPE_H */\n"));
    expect_mapped("1 after #endif", &w, 4);
    expect("1 hello.type.h", &w,
           "/* hello.type.h */\n#ifndef HELLO_TYPE_H\n#define HELLO_TYPE_H\n#include \"keel.type.h\"\n"
           "#line 2 \"hello.k\"\n#include <stdio.h>\n#endif /* HELLO_TYPE_H */\n");

    /* 2. consecutive lines 19, 20, 21: only the first needs #line
     *    (after #line 19 the counter gives 20, then 21). */
    start(&w, true);
    k_emit_at(&w, 19, S("a;\n"));
    k_emit_at(&w, 20, S("b;\n"));
    k_emit_at(&w, 21, S("c;\n"));
    expect_mapped("2", &w, 22);
    expect("2 consecutive", &w, "#line 19 \"hello.k\"\na;\nb;\nc;\n");

    /* 3. one .k line that becomes three output lines (the enum of `tags`):
     *    k_emit_at(12, 3 lines) -> mapped 15; line 18 differs -> #line 18. */
    start(&w, true);
    k_emit_at(&w, 12, S("e1\ne2\ne3\n"));
    expect_mapped("3 after expansion", &w, 15);
    k_emit_at(&w, 18, S("s;\n"));
    expect("3 expansion then resync", &w, "#line 12 \"hello.k\"\ne1\ne2\ne3\n#line 18 \"hello.k\"\ns;\n");

    /* 4. an injected line inside a body (the KEEL_CHECK of as_slice, §5.17 7a):
     *    header at 45 -> mapped 46; the raw check -> 47; the body line is 46,
     *    which differs -> #line 46; then 47 follows without #line. */
    start(&w, true);
    k_emit_at(&w, 45, S("f() {\n"));
    k_emit_raw(&w, S("    CHECK;\n"));
    expect_mapped("4 after injected", &w, 47);
    k_emit_at(&w, 46, S("    body;\n"));
    k_emit_at(&w, 47, S("}\n"));
    expect("4 injected", &w, "#line 45 \"hello.k\"\nf() {\n    CHECK;\n#line 46 \"hello.k\"\n    body;\n}\n");

    /* 5. backwards (prototypes, then bodies): 10, then 5 -> #line 5. */
    start(&w, true);
    k_emit_at(&w, 10, S("x;\n"));
    k_emit_at(&w, 5, S("y;\n"));
    expect("5 backwards", &w, "#line 10 \"hello.k\"\nx;\n#line 5 \"hello.k\"\ny;\n");

    /* 6. the .c of §8.3: synthetic include, then #line 1 and the source lines. */
    start(&w, true);
    k_emit_raw(&w, S("#include \"hello.h\"\n"));
    k_emit_at(&w, 1, S("\n\n\nint hello_main(void) {\n}\n"));
    expect_mapped("6", &w, 6);
    expect("6 .c opening", &w, "#include \"hello.h\"\n#line 1 \"hello.k\"\n\n\n\nint hello_main(void) {\n}\n");

    /* 7. --line=off: never a #line, whatever the lines. */
    start(&w, false);
    k_emit_at(&w, 2, S("x;\n"));
    k_emit_at(&w, 9, S("y;\n"));
    expect("7 line off", &w, "x;\ny;\n");

    /* 8. preconditions: text not ending in '\n' is refused and writes nothing;
     *    after a raw text that ends mid-line, k_emit_at is refused too. */
    start(&w, true);
    if (k_emit_at(&w, 3, S("no newline"))) { fprintf(stderr, "FAIL: 8 accepted text without newline\n"); failures++; }
    expect("8 nothing written", &w, "");
    k_emit_raw(&w, S("abc"));
    if (k_emit_at(&w, 3, S("x;\n"))) { fprintf(stderr, "FAIL: 8 accepted k_emit_at mid-line\n"); failures++; }
    expect("8 only the raw text", &w, "abc");

    /* 9. measuring and truncation: an empty slice counts the bytes; a slice
     *    of 10 holds exactly the first 10, and the length is still the full one. */
    const char *want9 = "#line 2 \"hello.k\"\nx;\n";   /* 18 + 3 = 21 bytes */
    k_writer_init(&w, (keel_slice_char){ 0, NULL }, S("hello.k"), true);
    k_emit_at(&w, 2, S("x;\n"));
    if (k_writer_len(&w) != strlen(want9)) { fprintf(stderr, "FAIL: 9 measured %zu, want %zu\n", k_writer_len(&w), strlen(want9)); failures++; }
    memset(buf, '#', sizeof buf);
    k_writer_init(&w, (keel_slice_char){ 10, buf }, S("hello.k"), true);
    k_emit_at(&w, 2, S("x;\n"));
    if (k_writer_len(&w) != strlen(want9)) { fprintf(stderr, "FAIL: 9 truncated length %zu, want %zu\n", k_writer_len(&w), strlen(want9)); failures++; }
    if (memcmp(buf, want9, 10) != 0 || buf[10] != '#') { fprintf(stderr, "FAIL: 9 the first 10 bytes, and nothing past them\n"); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    return 1;
}
