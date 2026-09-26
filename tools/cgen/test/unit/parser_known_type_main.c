/* Minimal oracle for the m2-parser-known-type task. Expected values are
 * hand-derived from the grammar/contract, not from a solving
 * implementation (tools/harness/README.md, "O oráculo não pode ser
 * resolvido escrevendo a resposta"). Not model-generated. */
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
    KSymbol storage[4];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 4);
    k_symtab_insert(&symtab, (keel_slice_char){ 6, (char *)"buffer" }, K_SYM_MODIFIER, 1);
    k_symtab_insert(&symtab, (keel_slice_char){ 5, (char *)"arena" }, K_SYM_TYPE, 0);

    KLexer lexer;
    TKPpKind pp;
    KToken next;

    /* Case 1: not in symtab at all — nothing consumed beyond `first`. */
    KToken first1 = first_token("xyz rest_token\n", &lexer);
    KSpecifier spec1;
    bool ok1 = k_scan_known_type(&lexer, first1, &symtab, &spec1, &next, &pp);
    if (!ok1) { fprintf(stderr, "FAIL: case1 — returned false\n"); failures++; }
    if (spec1.kind != K_SPEC_NONE) { fprintf(stderr, "FAIL: case1 — kind = %d, want K_SPEC_NONE\n", spec1.kind); failures++; }
    /* prove nothing was consumed: the lexer must still be sitting right
       after `first1`, so reading one more token here gets "rest_token". */
    KToken after1 = k_lexer_next(&lexer, &pp);
    if (!EQ(after1, "rest_token")) { fprintf(stderr, "FAIL: case1 — lexer advanced past 'xyz'; next real token = \"%.*s\", want \"rest_token\"\n", (int)after1.len, after1.ptr); failures++; }

    /* Case 2: registered modifier, arity 1, single-token argument. */
    KToken first2 = first_token("buffer i32 xs\n", &lexer);
    KSpecifier spec2;
    bool ok2 = k_scan_known_type(&lexer, first2, &symtab, &spec2, &next, &pp);
    if (!ok2) { fprintf(stderr, "FAIL: case2 — returned false\n"); failures++; }
    if (spec2.kind != K_SPEC_MODIFIER) { fprintf(stderr, "FAIL: case2 — kind = %d, want K_SPEC_MODIFIER\n", spec2.kind); failures++; }
    if (!EQ(spec2.modifier_name, "buffer")) { fprintf(stderr, "FAIL: case2 — modifier_name = \"%.*s\"\n", (int)spec2.modifier_name.len, spec2.modifier_name.ptr); failures++; }
    if (spec2.arg_count != 1) { fprintf(stderr, "FAIL: case2 — arg_count = %zu, want 1\n", spec2.arg_count); failures++; }
    else if (!EQ(spec2.args[0], "i32")) { fprintf(stderr, "FAIL: case2 — args[0] = \"%.*s\"\n", (int)spec2.args[0].len, spec2.args[0].ptr); failures++; }
    if (!EQ(next, "xs")) { fprintf(stderr, "FAIL: case2 — next_out = \"%.*s\", want \"xs\"\n", (int)next.len, next.ptr); failures++; }

    /* Case 3: registered named type, no arguments to consume. */
    KToken first3 = first_token("arena x\n", &lexer);
    KSpecifier spec3;
    bool ok3 = k_scan_known_type(&lexer, first3, &symtab, &spec3, &next, &pp);
    if (!ok3) { fprintf(stderr, "FAIL: case3 — returned false\n"); failures++; }
    if (spec3.kind != K_SPEC_NAMED_TYPE) { fprintf(stderr, "FAIL: case3 — kind = %d, want K_SPEC_NAMED_TYPE\n", spec3.kind); failures++; }
    if (!EQ(spec3.type_name, "arena")) { fprintf(stderr, "FAIL: case3 — type_name = \"%.*s\"\n", (int)spec3.type_name.len, spec3.type_name.ptr); failures++; }
    if (!EQ(next, "x")) { fprintf(stderr, "FAIL: case3 — next_out = \"%.*s\", want \"x\"\n", (int)next.len, next.ptr); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
