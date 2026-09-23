/* Hand-written driver for tools/harness/tasks/m1-lexer-scan-directive.md.
 * Not model-generated. */

#include <stdio.h>
#include <string.h>
#include "keel/keel_slice_char.type.h"

typedef enum { TK_PP_OTHER, TK_PP_IF, TK_PP_ELSE, TK_PP_ENDIF } TKPpKind;

size_t k_lexer_scan_directive(keel_slice_char source, size_t pos, TKPpKind *pp_kind_out);

static int failures = 0;

static keel_slice_char sl(const char *s, size_t n) {
    keel_slice_char v;
    v.ptr = (char *)s;
    v.len = n;
    return v;
}

static const char *kind_name(TKPpKind k) {
    switch (k) {
        case TK_PP_OTHER: return "OTHER";
        case TK_PP_IF: return "IF";
        case TK_PP_ELSE: return "ELSE";
        case TK_PP_ENDIF: return "ENDIF";
    }
    return "?";
}

static void check(size_t got_pos, size_t want_pos, TKPpKind got_kind, TKPpKind want_kind, const char *msg) {
    if (got_pos != want_pos) {
        fprintf(stderr, "FAIL: %s — pos got %zu, want %zu\n", msg, got_pos, want_pos);
        failures++;
    }
    if (got_kind != want_kind) {
        fprintf(stderr, "FAIL: %s — kind got %s, want %s\n", msg, kind_name(got_kind), kind_name(want_kind));
        failures++;
    }
}

int main(void) {
    {
        const char *s = "#if X\n";
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 6), 0, &k);
        check(p, 6, k, TK_PP_IF, "worked example");
    }
    {
        const char *s = "#ifdef X\n";
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 9), 0, &k);
        check(p, 9, k, TK_PP_IF, "ifdef classifies as TK_PP_IF");
    }
    {
        const char *s = "#ifndef X\n";
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 10), 0, &k);
        check(p, 10, k, TK_PP_IF, "ifndef classifies as TK_PP_IF");
    }
    {
        const char *s = "#elif X\n";
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 8), 0, &k);
        check(p, 8, k, TK_PP_ELSE, "elif classifies as TK_PP_ELSE");
    }
    {
        const char *s = "#else\n";
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 6), 0, &k);
        check(p, 6, k, TK_PP_ELSE, "else classifies as TK_PP_ELSE");
    }
    {
        const char *s = "#endif\n";
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 7), 0, &k);
        check(p, 7, k, TK_PP_ENDIF, "endif classifies as TK_PP_ENDIF");
    }
    {
        const char *s = "#define X 1\n";
        TKPpKind k = TK_PP_IF; /* deliberately wrong default, to catch a function that forgets to set it */
        size_t p = k_lexer_scan_directive(sl(s, 12), 0, &k);
        check(p, 12, k, TK_PP_OTHER, "define classifies as TK_PP_OTHER");
    }
    {
        /* space between # and the keyword */
        const char *s = "# if X\n";
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 7), 0, &k);
        check(p, 7, k, TK_PP_IF, "whitespace between # and the keyword");
    }
    {
        /* no newline at all: token runs to end of input */
        const char *s = "#endif";
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 6), 0, &k);
        check(p, 6, k, TK_PP_ENDIF, "no trailing newline: token runs to EOF");
    }
    {
        /* a long, unknown word must not overrun the classification buffer */
        const char *s = "#abcdefghijklmnop\n";
        TKPpKind k = TK_PP_IF;
        size_t p = k_lexer_scan_directive(sl(s, 18), 0, &k);
        check(p, 18, k, TK_PP_OTHER, "long unknown word does not overflow the keyword buffer");
    }
    {
        /* splice inside the keyword itself */
        const char *s = "#en\\\ndif\n"; /* # e n \ \n d i f \n = 9 bytes */
        TKPpKind k = TK_PP_OTHER;
        size_t p = k_lexer_scan_directive(sl(s, 9), 0, &k);
        check(p, 9, k, TK_PP_ENDIF, "a splice inside the keyword is transparent to classification");
    }
    {
        /* # with nothing recognizable after it at all */
        const char *s = "#\n";
        TKPpKind k = TK_PP_IF;
        size_t p = k_lexer_scan_directive(sl(s, 2), 0, &k);
        check(p, 2, k, TK_PP_OTHER, "bare # is TK_PP_OTHER");
    }

    if (failures == 0) {
        puts("ok");
        return 0;
    }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
