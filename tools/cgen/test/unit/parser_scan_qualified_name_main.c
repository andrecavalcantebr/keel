/* Unit test of engine/, first written as the oracle of the harness task
 * m2-parser-scan-qualified-name.md. Not model-generated. */

#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;

static void check(const char *label, const char *src,
                   const char *want_name, const char *want_next_spelling,
                   int want_next_len) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    k_lexer_init(&lexer, source, NULL);

    TKPpKind pp;
    KToken first = k_lexer_next(&lexer, &pp);

    KToken next;
    TKPpKind next_pp;
    keel_slice_char name = k_scan_qualified_name(&lexer, first, &next, &next_pp);

    size_t want_len = strlen(want_name);
    if (name.len != want_len || memcmp(name.ptr, want_name, want_len) != 0) {
        fprintf(stderr, "FAIL: %s — name = \"%.*s\" (%zu bytes), want \"%s\" (%zu bytes)\n",
                label, (int)name.len, name.ptr, name.len, want_name, want_len);
        failures++;
    }

    if ((int)next.len != want_next_len) {
        fprintf(stderr, "FAIL: %s — next.len = %zu, want %d\n", label, next.len, want_next_len);
        failures++;
    } else if (want_next_len > 0 &&
               memcmp(next.ptr, want_next_spelling, (size_t)want_next_len) != 0) {
        fprintf(stderr, "FAIL: %s — next = \"%.*s\", want \"%s\"\n",
                label, (int)next.len, next.ptr, want_next_spelling);
        failures++;
    }
}

int main(void) {
    check("single segment", "hello;", "hello", ";", 1);
    check("two segments", "app.cfg;", "app.cfg", ";", 1);
    check("three segments", "a.b.c;", "a.b.c", ";", 1);
    /* trivia between tokens is part of the slice, not stripped — worked in
       the task as: "app . cfg" is 9 bytes (a,p,p,sp,.,sp,c,f,g), and the
       trailing space before ';' is NOT included, because it comes after the
       last IDENT of the name. */
    check("trivia preserved, not the trailing space", "app . cfg ;", "app . cfg", ";", 1);
    /* truncated file: the name is exactly the file, and `next` is EOF (an
       empty token) — not a crash, not a read past the buffer (ASan would
       catch that). */
    check("EOF right after the name", "app", "app", "", 0);

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
