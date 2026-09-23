/* Hand-written driver for tools/harness/tasks/m1-lexer-scan-number.md.
 * Not model-generated. */

#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

size_t k_lexer_scan_number(keel_slice_char source, size_t pos);

static int failures = 0;

static keel_slice_char sl(const char *s, size_t n) {
    keel_slice_char v;
    v.ptr = (char *)s;
    v.len = n;
    return v;
}

#define CHECK_EQ(got, want, msg) do { \
        size_t g_ = (got), w_ = (want); \
        if (g_ != w_) { \
            fprintf(stderr, "FAIL: %s — got %zu, want %zu\n", msg, g_, w_); \
            failures++; \
        } \
    } while (0)

int main(void) {
    { const char *s = "42"; CHECK_EQ(k_lexer_scan_number(sl(s, 2), 0), 2, "plain integer"); }

    { const char *s = "2..7"; CHECK_EQ(k_lexer_scan_number(sl(s, 4), 0), 1, "worked example: 2..7 stops before .."); }

    { const char *s = "1e-6"; CHECK_EQ(k_lexer_scan_number(sl(s, 4), 0), 4, "worked example: 1e-6 whole thing"); }

    { const char *s = "0x1.fp3"; CHECK_EQ(k_lexer_scan_number(sl(s, 7), 0), 7, "worked example: 0x1.fp3 whole thing"); }

    { const char *s = "1.5..3"; CHECK_EQ(k_lexer_scan_number(sl(s, 6), 0), 3, "1.5..3 stops after 1.5, before .."); }

    { const char *s = "2...7"; CHECK_EQ(k_lexer_scan_number(sl(s, 5), 0), 1, "2...7 stops before the ellipsis"); }

    { const char *s = ".5"; CHECK_EQ(k_lexer_scan_number(sl(s, 2), 0), 2, "leading-dot number"); }

    { const char *s = "1abc xyz"; CHECK_EQ(k_lexer_scan_number(sl(s, 8), 0), 4, "permissive: letters after digits all consumed"); }

    { const char *s = "1+2"; CHECK_EQ(k_lexer_scan_number(sl(s, 3), 0), 1, "bare + not after e/E/p/P stops the number"); }

    { const char *s = "0p+3"; CHECK_EQ(k_lexer_scan_number(sl(s, 4), 0), 4, "+ right after p is consumed"); }

    /* splice inside a number */
    { const char *s = "1\\\ne2"; CHECK_EQ(k_lexer_scan_number(sl(s, 5), 0), 5, "a splice inside a number is transparent"); }

    /* starting mid-buffer */
    { const char *s = "  42x "; CHECK_EQ(k_lexer_scan_number(sl(s, 6), 2), 5, "pos does not have to be 0"); }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
