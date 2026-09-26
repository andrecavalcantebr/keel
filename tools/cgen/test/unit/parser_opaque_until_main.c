/* Minimal oracle for the m2-parser-opaque-until task. Expected values
 * hand-derived by tracing depth through the input. Not model-generated.
 *
 * Trace for "f(a,b) ; ok", terminators = {",", ";"}, first = "f":
 *   f: depth 0, no match, not a delimiter -> depth stays 0
 *   (: depth 0, no match, opens          -> depth becomes 1
 *   a: depth 1 (skip check), not delim   -> depth stays 1
 *   ,: depth 1 (skip check) -- this comma must NOT be treated as a
 *      terminator, it is inside the parens
 *   b: depth 1, not delim                -> depth stays 1
 *   ): depth 1 (skip check), closes      -> depth becomes 0
 *   ;: depth 0, MATCHES terminators[1]   -> stop here
 * next_out = ";" itself (already read); one more k_lexer_next() call
 * (done by the test, simulating the caller) must then read "ok".
 */
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
    size_t idx;
    static const char *terms[] = { ",", ";" };

    /* balanced skip past a nested comma */
    KToken first1 = first_token("f(a,b) ; ok\n", &lexer);
    k_scan_opaque_until(&lexer, first1, terms, 2, &idx, &next, &pp);
    if (idx != 1) { fprintf(stderr, "FAIL: case1 — terminator_index = %zu, want 1 (';')\n", idx); failures++; }
    if (!EQ(next, ";")) { fprintf(stderr, "FAIL: case1 — next_out = \"%.*s\", want \";\"\n", (int)next.len, next.ptr); failures++; }
    KToken after1 = k_lexer_next(&lexer, &pp);
    if (!EQ(after1, "ok")) { fprintf(stderr, "FAIL: case1 — token after ';' = \"%.*s\", want \"ok\"\n", (int)after1.len, after1.ptr); failures++; }

    /* first token is itself the terminator */
    KToken first2 = first_token(", rest\n", &lexer);
    k_scan_opaque_until(&lexer, first2, terms, 2, &idx, &next, &pp);
    if (idx != 0) { fprintf(stderr, "FAIL: case2 — terminator_index = %zu, want 0 (',')\n", idx); failures++; }
    if (!EQ(next, ",")) { fprintf(stderr, "FAIL: case2 — next_out = \"%.*s\", want \",\"\n", (int)next.len, next.ptr); failures++; }
    KToken after2 = k_lexer_next(&lexer, &pp);
    if (!EQ(after2, "rest")) { fprintf(stderr, "FAIL: case2 — token after ',' = \"%.*s\", want \"rest\"\n", (int)after2.len, after2.ptr); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
