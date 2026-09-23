/* Hand-written driver for tools/harness/tasks/m1-lexer-scan-quoted.md.
 * Not model-generated. */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

size_t k_lexer_scan_quoted(keel_slice_char source, size_t pos, bool *unterminated_out);

static int failures = 0;

static keel_slice_char sl(const char *s, size_t n) {
    keel_slice_char v;
    v.ptr = (char *)s;
    v.len = n;
    return v;
}

static void check(size_t got_pos, size_t want_pos, bool got_unterm, bool want_unterm, const char *msg) {
    if (got_pos != want_pos) {
        fprintf(stderr, "FAIL: %s — pos got %zu, want %zu\n", msg, got_pos, want_pos);
        failures++;
    }
    if (got_unterm != want_unterm) {
        fprintf(stderr, "FAIL: %s — unterminated got %d, want %d\n", msg, got_unterm, want_unterm);
        failures++;
    }
}

int main(void) {
    {
        const char *s = "\"ab\"";
        bool u = true;
        size_t p = k_lexer_scan_quoted(sl(s, 4), 0, &u);
        check(p, 4, u, false, "plain string literal");
    }
    {
        const char *s = "'a\\'b'";
        bool u = true;
        size_t p = k_lexer_scan_quoted(sl(s, 6), 0, &u);
        check(p, 6, u, false, "worked example: escaped quote does not close it");
    }
    {
        const char *s = "\"ab\ncd\"";
        bool u = false;
        size_t p = k_lexer_scan_quoted(sl(s, 7), 0, &u);
        check(p, 3, u, true, "worked example: raw newline is unterminated");
    }
    {
        const char *s = "u8\"hi\"x";
        bool u = true;
        size_t p = k_lexer_scan_quoted(sl(s, 7), 0, &u);
        check(p, 6, u, false, "u8 prefix");
    }
    {
        const char *s = "u'x'y";
        bool u = true;
        size_t p = k_lexer_scan_quoted(sl(s, 5), 0, &u);
        check(p, 4, u, false, "u prefix on a char literal");
    }
    {
        const char *s = "L\"wide\"z";
        bool u = true;
        size_t p = k_lexer_scan_quoted(sl(s, 8), 0, &u);
        check(p, 7, u, false, "L prefix");
    }
    {
        /* backslash-backslash: the second backslash does not start a new escape */
        const char *s = "\"a\\\\\"b"; /* "a\\" then b -> a \\ closes the string */
        bool u = true;
        size_t p = k_lexer_scan_quoted(sl(s, 6), 0, &u);
        check(p, 5, u, false, "double backslash is one escape pair, then the quote closes normally");
    }
    {
        /* unterminated at end of input, no newline at all */
        const char *s = "\"abc";
        bool u = false;
        size_t p = k_lexer_scan_quoted(sl(s, 4), 0, &u);
        check(p, 4, u, true, "unterminated at end of input");
    }
    {
        /* a trailing lone backslash right at EOF, inside an unterminated literal */
        const char *s = "\"abc\\";
        bool u = false;
        size_t p = k_lexer_scan_quoted(sl(s, 5), 0, &u);
        check(p, 5, u, true, "trailing lone backslash then EOF does not crash, still unterminated");
    }
    {
        /* unterminated_out may be NULL */
        const char *s = "\"ok\"";
        size_t p = k_lexer_scan_quoted(sl(s, 4), 0, NULL);
        if (p != 4) { fprintf(stderr, "FAIL: works with unterminated_out == NULL — got %zu, want 4\n", p); failures++; }
    }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
