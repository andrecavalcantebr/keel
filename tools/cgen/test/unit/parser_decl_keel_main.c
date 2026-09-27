/* Minimal oracle for the m2-parser-decl-keel task. Expected values
 * hand-derived by tracing the composition of k_scan_known_type +
 * k_scan_declarator_head + k_scan_opaque_until. Not model-generated.
 *
 * Trace for "buffer i32 xs; ok" (symtab: "buffer" = MODIFIER, arity 1):
 *   k_scan_known_type("buffer") -> MODIFIER, arg[0]="i32", next="xs"
 *   k_scan_declarator_head("xs") -> name="xs", next=";"
 *   register "xs" as VARIABLE; ';' is not '=' and not ',' -> done
 *   one more read past ';' -> next_out = "ok"
 *
 * Trace for "arena a = f(1,2), b; ok2" (symtab: "arena" = TYPE):
 *   k_scan_known_type("arena") -> NAMED_TYPE, next="a"
 *   k_scan_declarator_head("a") -> name="a", next="="
 *   register "a"; sees '=' -> read initializer's first token "f", then
 *     k_scan_opaque_until("f", {",",";"}) skips "(1,2)" as one balanced
 *     unit (the comma inside it is at depth 1, not a match) and stops at
 *     the SECOND ',' (depth 0) -> that ',' means another init-decl follows
 *   k_scan_declarator_head("b") -> name="b", next=";"
 *   register "b"; ';' ends it -> one more read past ';' -> next_out = "ok2"
 *
 * Trace for "unregistered rest" (not in symtab at all):
 *   k_scan_known_type -> K_SPEC_NONE -> k_scan_decl_keel returns false,
 *   nothing else touched.
 */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
/* len 0 short-circuits: a wrong implementation may leave .ptr NULL,
   and memcmp(NULL, ..., 0) is undefined — it must report FAIL, not
   abort, because the FAIL text is what a reader (or the harness
   loop) sees. */
#define EQ(s, want) (strlen(want) == (s).len && \
                     ((s).len == 0 || memcmp((s).ptr, want, (s).len) == 0))

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
    KSymbol storage[16];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 16);
    k_symtab_insert(&symtab, (keel_slice_char){ 6, (char *)"buffer" }, K_SYM_MODIFIER, 1);
    k_symtab_insert(&symtab, (keel_slice_char){ 5, (char *)"arena" }, K_SYM_TYPE, 0);
    /* `i32` has to be here: keel-spec §2.2 says `named-type` "só casa um
       nome registrado como tipo", so `buffer i32` is a known-type only
       when i32 is one. Registering just `buffer` and `arena` made this
       oracle assert that an unregistered argument is accepted, which the
       grammar never said. */
    k_symtab_insert(&symtab, (keel_slice_char){ 3, (char *)"i32" }, K_SYM_TYPE, 0);

    /* case 1: modifier specifier, single name, no initializer */
    KToken first1 = first_token("buffer i32 xs; ok\n", &lexer);
    KKeelDecl kd1;
    bool ok1 = k_scan_decl_keel(&lexer, first1, &symtab, &kd1, &next, &pp);
    if (!ok1) { fprintf(stderr, "FAIL: case1 — returned false\n"); failures++; }
    if (kd1.spec.kind != K_SPEC_MODIFIER) { fprintf(stderr, "FAIL: case1 — spec.kind = %d, want K_SPEC_MODIFIER\n", kd1.spec.kind); failures++; }
    if (kd1.name_count != 1) { fprintf(stderr, "FAIL: case1 — name_count = %zu, want 1\n", kd1.name_count); failures++; }
    else if (!EQ(kd1.names[0].name, "xs")) { fprintf(stderr, "FAIL: case1 — names[0] = \"%.*s\"\n", (int)kd1.names[0].name.len, kd1.names[0].name.ptr); failures++; }
    if (!EQ(next, "ok")) { fprintf(stderr, "FAIL: case1 — next_out = \"%.*s\", want \"ok\"\n", (int)next.len, next.ptr); failures++; }
    if (symtab.count != 4) { fprintf(stderr, "FAIL: case1 — symtab.count = %zu, want 4\n", symtab.count); failures++; }

    /* case 2: named-type specifier, two names, one with an initializer
       containing a nested comma that must not be mistaken for the
       init-decl separator */
    KToken first2 = first_token("arena a = f(1,2), b; ok2\n", &lexer);
    KKeelDecl kd2;
    bool ok2 = k_scan_decl_keel(&lexer, first2, &symtab, &kd2, &next, &pp);
    if (!ok2) { fprintf(stderr, "FAIL: case2 — returned false\n"); failures++; }
    if (kd2.spec.kind != K_SPEC_NAMED_TYPE) { fprintf(stderr, "FAIL: case2 — spec.kind = %d, want K_SPEC_NAMED_TYPE\n", kd2.spec.kind); failures++; }
    if (kd2.name_count != 2) { fprintf(stderr, "FAIL: case2 — name_count = %zu, want 2\n", kd2.name_count); failures++; }
    else {
        if (!EQ(kd2.names[0].name, "a")) { fprintf(stderr, "FAIL: case2 — names[0] = \"%.*s\"\n", (int)kd2.names[0].name.len, kd2.names[0].name.ptr); failures++; }
        if (!EQ(kd2.names[1].name, "b")) { fprintf(stderr, "FAIL: case2 — names[1] = \"%.*s\"\n", (int)kd2.names[1].name.len, kd2.names[1].name.ptr); failures++; }
    }
    if (!EQ(next, "ok2")) { fprintf(stderr, "FAIL: case2 — next_out = \"%.*s\", want \"ok2\"\n", (int)next.len, next.ptr); failures++; }
    if (symtab.count != 6) { fprintf(stderr, "FAIL: case2 — symtab.count = %zu, want 6\n", symtab.count); failures++; }

    /* case 3: not a keel specifier at all */
    KToken first3 = first_token("unregistered rest\n", &lexer);
    KKeelDecl kd3;
    bool ok3 = k_scan_decl_keel(&lexer, first3, &symtab, &kd3, &next, &pp);
    if (ok3) { fprintf(stderr, "FAIL: case3 — returned true, want false\n"); failures++; }
    if (symtab.count != 6) { fprintf(stderr, "FAIL: case3 — must not register anything, symtab.count = %zu, want 6\n", symtab.count); failures++; }

    /* case 4: a declarator with an array suffix. Reading only the
       declarator's head left next_out on the '4' INSIDE the brackets and
       still returned true — the same failure mode decl-tags had, and the
       reason this case exists. */
    KToken first4 = first_token("arena a[4]; ok4\n", &lexer);
    KKeelDecl kd4;
    bool ok4 = k_scan_decl_keel(&lexer, first4, &symtab, &kd4, &next, &pp);
    if (!ok4) { fprintf(stderr, "FAIL: case4 — returned false\n"); failures++; }
    if (kd4.name_count != 1 || !EQ(kd4.names[0].name, "a")) { fprintf(stderr, "FAIL: case4 — names wrong\n"); failures++; }
    if (!EQ(next, "ok4")) { fprintf(stderr, "FAIL: case4 — next_out = \"%.*s\", want \"ok4\"\n", (int)next.len, next.ptr); failures++; }

    /* case 5: pointer and suffix in the same comma list */
    KToken first5 = first_token("arena *p, q[2]; ok5\n", &lexer);
    KKeelDecl kd5;
    bool ok5 = k_scan_decl_keel(&lexer, first5, &symtab, &kd5, &next, &pp);
    if (!ok5) { fprintf(stderr, "FAIL: case5 — returned false\n"); failures++; }
    if (kd5.name_count != 2 || !EQ(kd5.names[0].name, "p") || !EQ(kd5.names[1].name, "q")) {
        fprintf(stderr, "FAIL: case5 — names wrong\n"); failures++;
    }
    if (!EQ(next, "ok5")) { fprintf(stderr, "FAIL: case5 — next_out = \"%.*s\", want \"ok5\"\n", (int)next.len, next.ptr); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
