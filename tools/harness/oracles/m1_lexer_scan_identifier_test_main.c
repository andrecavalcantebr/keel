/* Hand-written driver for tools/harness/tasks/m1-lexer-scan-identifier.md.
 * Not model-generated. */

#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

size_t k_lexer_scan_identifier(keel_slice_char source, size_t pos);

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
    { const char *s = "my_var2 x"; CHECK_EQ(k_lexer_scan_identifier(sl(s, 9), 0), 7, "plain identifier stops at the space"); }

    { const char *s = "x"; CHECK_EQ(k_lexer_scan_identifier(sl(s, 1), 0), 1, "single-char identifier at EOF"); }

    { const char *s = "_priv;"; CHECK_EQ(k_lexer_scan_identifier(sl(s, 6), 0), 5, "underscore start, stops at semicolon"); }

    { const char *s = "abc123"; CHECK_EQ(k_lexer_scan_identifier(sl(s, 6), 0), 6, "letters then digits, whole thing"); }

    /* splice inside an identifier: the task's own worked example */
    { const char *s = "my\\\n_x"; CHECK_EQ(k_lexer_scan_identifier(sl(s, 6), 0), 6, "worked example: splice inside an identifier"); }

    /* a 4-hex-digit universal character name */
    { const char *s = "\\u00e9x y"; CHECK_EQ(k_lexer_scan_identifier(sl(s, 9), 0), 7, "\\u UCN then a letter, stops at the space"); }

    /* an 8-hex-digit universal character name */
    { const char *s = "\\U0001F600z "; CHECK_EQ(k_lexer_scan_identifier(sl(s, 12), 0), 11, "\\U UCN then a letter, stops at the space"); }

    /* malformed UCN: too few hex digits after \u — stops before the backslash */
    { const char *s = "a\\u12 "; CHECK_EQ(k_lexer_scan_identifier(sl(s, 6), 0), 1, "malformed \\u stops the scan right before it"); }

    /* scanning starting mid-buffer, not just at offset 0 */
    { const char *s = "  abc "; CHECK_EQ(k_lexer_scan_identifier(sl(s, 6), 2), 5, "pos does not have to be 0"); }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
