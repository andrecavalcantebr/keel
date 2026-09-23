/* Hand-written driver for tools/harness/tasks/m1-lexer-peek.md. Not
 * model-generated. */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

size_t k_lexer_splice_width(keel_slice_char source, size_t pos);
int k_lexer_peek_at(keel_slice_char source, size_t pos, size_t *width_out);

static int failures = 0;

static keel_slice_char sl(const char *s, size_t n) {
    keel_slice_char v;
    v.ptr = (char *)s;
    v.len = n;
    return v;
}

#define CHECK(cond, msg) do { \
        if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
    } while (0)

#define CHECK_EQ_SIZE(got, want, msg) do { \
        size_t g_ = (got), w_ = (want); \
        if (g_ != w_) { \
            fprintf(stderr, "FAIL: %s — got %zu, want %zu\n", msg, g_, w_); \
            failures++; \
        } \
    } while (0)

int main(void) {
    /* k_lexer_splice_width */
    CHECK(k_lexer_splice_width(sl("\\\nx", 3), 0) == 2, "backslash-LF is a 2-byte splice");
    CHECK(k_lexer_splice_width(sl("\\\r\nx", 4), 0) == 3, "backslash-CRLF is a 3-byte splice");
    CHECK(k_lexer_splice_width(sl("\\\rx", 3), 0) == 2, "backslash-CR is a 2-byte splice");
    CHECK(k_lexer_splice_width(sl("\\x", 2), 0) == 0, "backslash-x is not a splice");
    CHECK(k_lexer_splice_width(sl("\\", 1), 0) == 0, "trailing lone backslash is not a splice");
    CHECK(k_lexer_splice_width(sl("ab", 2), 0) == 0, "no backslash at all");
    CHECK(k_lexer_splice_width(sl("a\\\n", 3), 1) == 2, "splice_width honors pos, not just offset 0");

    /* k_lexer_peek_at: plain byte, no splice */
    {
        size_t w = 999;
        int c = k_lexer_peek_at(sl("ab", 2), 0, &w);
        CHECK(c == 'a', "peek plain byte value");
        CHECK_EQ_SIZE(w, 1, "peek plain byte width");
    }

    /* one splice then a real character */
    {
        size_t w = 999;
        int c = k_lexer_peek_at(sl("\\\nx", 3), 0, &w);
        CHECK(c == 'x', "peek skips a single splice to reach x");
        CHECK_EQ_SIZE(w, 3, "peek width covers backslash+LF+x");
    }

    /* CRLF splice then a real character */
    {
        size_t w = 999;
        int c = k_lexer_peek_at(sl("\\\r\ny", 4), 0, &w);
        CHECK(c == 'y', "peek skips a CRLF splice to reach y");
        CHECK_EQ_SIZE(w, 4, "peek width covers backslash+CR+LF+y");
    }

    /* two chained splices then a real character */
    {
        size_t w = 999;
        int c = k_lexer_peek_at(sl("\\\n\\\nz", 5), 0, &w);
        CHECK(c == 'z', "peek skips two chained splices to reach z");
        CHECK_EQ_SIZE(w, 5, "peek width covers both splices plus z");
    }

    /* empty source: immediate EOF */
    {
        size_t w = 999;
        int c = k_lexer_peek_at(sl("", 0), 0, &w);
        CHECK(c == -1, "peek on empty source is EOF");
        CHECK_EQ_SIZE(w, 0, "EOF width is 0");
    }

    /* a splice right at the end of input: EOF after consuming it */
    {
        size_t w = 999;
        int c = k_lexer_peek_at(sl("\\\n", 2), 0, &w);
        CHECK(c == -1, "peek at a trailing splice with nothing after is EOF");
        CHECK_EQ_SIZE(w, 0, "EOF width is 0, even after a splice was involved");
    }

    /* a lone trailing backslash (NOT a splice) is an ordinary logical byte */
    {
        size_t w = 999;
        int c = k_lexer_peek_at(sl("\\", 1), 0, &w);
        CHECK(c == '\\', "a lone trailing backslash is itself the logical byte");
        CHECK_EQ_SIZE(w, 1, "width 1 for the ordinary backslash byte");
    }

    /* width_out may be NULL */
    {
        int c = k_lexer_peek_at(sl("q", 1), 0, NULL);
        CHECK(c == 'q', "peek works with width_out == NULL");
    }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
