/* Hand-written driver for tools/harness/tasks/m1-lexer-scan-punct.md.
 * Not model-generated. */

#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

size_t k_lexer_scan_punct(keel_slice_char source, size_t pos);

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
    { const char *s = "<<=x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 4), 0), 3, "worked example: <<= is 3-byte"); }

    { const char *s = "<\\\n<x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 5), 0), 4, "worked example: << across a splice"); }

    { const char *s = "@x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 2), 0), 1, "worked example: unrecognized byte falls back to 1"); }

    { const char *s = "...x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 4), 0), 3, "ellipsis, 3 dots"); }
    { const char *s = "..x";  CHECK_EQ(k_lexer_scan_punct(sl(s, 3), 0), 2, "keel range, 2 dots"); }
    { const char *s = ".x";   CHECK_EQ(k_lexer_scan_punct(sl(s, 2), 0), 1, "member access, 1 dot"); }

    { const char *s = "->x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 3), 0), 2, "arrow"); }
    { const char *s = "==x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 3), 0), 2, "equality"); }
    { const char *s = "&&x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 3), 0), 2, "logical and"); }
    { const char *s = "##x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 3), 0), 2, "token paste"); }

    { const char *s = "<<"; CHECK_EQ(k_lexer_scan_punct(sl(s, 2), 0), 2, "<< with nothing after it (only 2 bytes total)"); }
    { const char *s = "<";  CHECK_EQ(k_lexer_scan_punct(sl(s, 1), 0), 1, "a single < at end of input"); }
    { const char *s = "+";  CHECK_EQ(k_lexer_scan_punct(sl(s, 1), 0), 1, "a single + at end of input"); }

    { const char *s = "[x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 2), 0), 1, "single-char bracket"); }
    { const char *s = ";x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 2), 0), 1, "semicolon"); }

    /* starting mid-buffer */
    { const char *s = "  ->x"; CHECK_EQ(k_lexer_scan_punct(sl(s, 5), 2), 4, "pos does not have to be 0"); }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
