/* Minimal oracle for the m2-parser-declarator-head task. Expected values
 * are hand-derived from the grammar, not from a solving implementation.
 * Not model-generated. */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && memcmp((s).ptr, want, (s).len) == 0)

static KToken first_token(const char *src, KLexer *lexer) {
    keel_slice_char source = { strlen(src), (char *)src };
    TKPpKind pp;
    k_lexer_init(lexer, source, NULL);
    return k_lexer_next(lexer, &pp);
}

int main(void) {
    KLexer lexer;
    TKPpKind pp;
    KToken next;

    /* no stars */
    KToken first1 = first_token("p rest1\n", &lexer);
    KDeclaratorHead d1;
    bool ok1 = k_scan_declarator_head(&lexer, first1, &d1, &next, &pp);
    if (!ok1) { fprintf(stderr, "FAIL: case1 — returned false\n"); failures++; }
    if (d1.pointer_depth != 0) { fprintf(stderr, "FAIL: case1 — pointer_depth = %d, want 0\n", d1.pointer_depth); failures++; }
    if (!EQ(d1.name, "p")) { fprintf(stderr, "FAIL: case1 — name = \"%.*s\"\n", (int)d1.name.len, d1.name.ptr); failures++; }
    if (!EQ(next, "rest1")) { fprintf(stderr, "FAIL: case1 — next_out = \"%.*s\", want \"rest1\"\n", (int)next.len, next.ptr); failures++; }

    /* two stars */
    KToken first2 = first_token("* * name rest2\n", &lexer);
    KDeclaratorHead d2;
    bool ok2 = k_scan_declarator_head(&lexer, first2, &d2, &next, &pp);
    if (!ok2) { fprintf(stderr, "FAIL: case2 — returned false\n"); failures++; }
    if (d2.pointer_depth != 2) { fprintf(stderr, "FAIL: case2 — pointer_depth = %d, want 2\n", d2.pointer_depth); failures++; }
    if (!EQ(d2.name, "name")) { fprintf(stderr, "FAIL: case2 — name = \"%.*s\"\n", (int)d2.name.len, d2.name.ptr); failures++; }
    if (!EQ(next, "rest2")) { fprintf(stderr, "FAIL: case2 — next_out = \"%.*s\", want \"rest2\"\n", (int)next.len, next.ptr); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
