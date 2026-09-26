/* Minimal oracle for the m2-parser-ident-list task. Not model-generated. */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && memcmp((s).ptr, want, (s).len) == 0)

int main(void) {
    keel_slice_char src = { 0 };
    const char *s = "a, b, c ok";
    src.len = strlen(s); src.ptr = (char *)s;
    KLexer lexer; TKPpKind pp;
    k_lexer_init(&lexer, src, NULL);
    KToken first = k_lexer_next(&lexer, &pp);

    KToken out[8]; size_t count; KToken next; TKPpKind npp;
    bool ok = k_scan_ident_list(&lexer, first, out, 8, &count, &next, &npp);

    if (!ok) { fprintf(stderr, "FAIL: returned false\n"); failures++; }
    if (count != 3) { fprintf(stderr, "FAIL: count = %zu, want 3\n", count); failures++; }
    else {
        if (!EQ(out[0], "a")) { fprintf(stderr, "FAIL: out[0] = \"%.*s\"\n", (int)out[0].len, out[0].ptr); failures++; }
        if (!EQ(out[1], "b")) { fprintf(stderr, "FAIL: out[1] = \"%.*s\"\n", (int)out[1].len, out[1].ptr); failures++; }
        if (!EQ(out[2], "c")) { fprintf(stderr, "FAIL: out[2] = \"%.*s\"\n", (int)out[2].len, out[2].ptr); failures++; }
    }
    if (!EQ(next, "ok")) { fprintf(stderr, "FAIL: next_out = \"%.*s\", want \"ok\"\n", (int)next.len, next.ptr); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
