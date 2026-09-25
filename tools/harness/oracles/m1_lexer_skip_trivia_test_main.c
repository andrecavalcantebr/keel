/* Hand-written driver for tools/harness/tasks/m1-lexer-skip-trivia.md.
 * Not model-generated. */

#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

size_t k_lexer_skip_trivia(keel_slice_char source, size_t pos);

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
    CHECK_EQ(k_lexer_skip_trivia(sl("", 0), 0), 0, "empty input");

    { const char *s = "   "; CHECK_EQ(k_lexer_skip_trivia(sl(s, 3), 0), 3, "pure horizontal whitespace"); }

    { const char *s = " \tx"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 3), 0), 2, "space+tab then a real byte"); }

    /* // comment, stops right before the newline, does not consume it */
    { const char *s = "  //x\n/**/y"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 11), 0), 5, "worked example from the task"); }

    /* second call starting past the newline picks up the block comment */
    { const char *s = "  //x\n/**/y"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 11), 6), 10, "block comment after the newline"); }

    /* // comment with nothing after it: consumes to EOF */
    { const char *s = "//abc"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 5), 0), 5, "line comment with no trailing newline"); }

    /* closed block comment */
    { const char *s = "/* hi */x"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 9), 0), 8, "closed block comment stops right before x"); }

    /* unterminated block comment: consumes to EOF, no crash */
    { const char *s = "/* never closes"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 15), 0), 15, "unterminated block comment reaches EOF"); }

    /* block comments do not nest: the first closing pair wins, and the
       space right after it is itself ordinary trivia that keeps getting
       consumed — only the second '*' (not part of any comment opener or
       horizontal whitespace) stops the loop */
    { const char *s = "/* /* */ */"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 11), 0), 9, "block comments do not nest"); }

    /* a splice inside a // comment extends it across the physical line */
    {
        const char *s = "//a\\\nb\n"; /* / / a \ \n b \n  -> 7 bytes */
        CHECK_EQ(k_lexer_skip_trivia(sl(s, 7), 0), 6, "a splice inside // keeps the comment going onto the next physical line");
    }

    /* already at end of input */
    /* a splice may sit before any byte of a comment delimiter: each byte
       has its own width */
    { const char *s = "/* x \\\n*/y"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 10), 0), 9, "`*` splice `/` closes the block comment"); }
    { const char *s = "/*x*\\\n/y"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 8), 0), 7, "`*` then splice then `/` closes it too"); }
    { const char *s = "/\\\r\n* x */y"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 11), 0), 10, "`/` splice(CRLF) `*` opens a block comment"); }
    { const char *s = "\\\n//x\ny"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 7), 0), 5, "a splice before `//` still starts a line comment"); }
    { const char *s = "/* x *\\\n"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 8), 0), 8, "unclosed, ending in a splice: EOF"); }

    { const char *s = "x"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 1), 1), 1, "pos already at source.len"); }

    /* a single '/' that is not the start of any comment is not trivia */
    { const char *s = "/x"; CHECK_EQ(k_lexer_skip_trivia(sl(s, 2), 0), 0, "a lone slash is not trivia"); }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
