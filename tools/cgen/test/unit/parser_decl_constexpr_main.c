/* Minimal oracle for the m2-parser-decl-constexpr task. Expected values
 * hand-derived from the (revised) grammar, not from a solving
 * implementation. Not model-generated.
 *
 * Trace for "size_t MAX = 256; ok" ('constexpr' already consumed):
 *   size_t (not '=' or ';', depth 0) -> prev = size_t
 *   MAX (not '=' or ';') -> prev = MAX
 *   = (depth 0, matches!) -> prev (MAX) is IDENT -> has_name, name = MAX;
 *     register MAX as K_SYM_CONSTANT. Skip initializer: read "256", then
 *     k_scan_opaque_until(..., {";"}) stops at ";" -> one more read = "ok"
 *
 * Trace for "unsigned long flag; ok2" (no '=' at all):
 *   unsigned -> prev; long -> prev; flag -> prev
 *   ; (depth 0, matches) -> no '=' was ever seen -> has_name = false
 *   one more read past ';' -> next_out = "ok2"; nothing registered
 *
 * Trace for "= 5; oops" (degenerate: '=' is the very first token, no
 * token read before it at all):
 *   = (depth 0, matches, but there is no previous token) -> constexpr-
 *     name-missing -> return false, nothing touched beyond `first`
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
    KSymbol storage[4];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 4);

    /* case 1: has a name */
    KToken kw = first_token("constexpr size_t MAX = 256; ok\n", &lexer); /* kw is 'constexpr' itself */
    KConstexprDecl c1;
    bool ok1 = k_scan_decl_constexpr(&lexer, kw, &symtab, &c1, &next, &pp);
    if (!ok1) { fprintf(stderr, "FAIL: case1 — returned false\n"); failures++; }
    if (!c1.has_name) { fprintf(stderr, "FAIL: case1 — has_name = false\n"); failures++; }
    if (!EQ(c1.name, "MAX")) { fprintf(stderr, "FAIL: case1 — name = \"%.*s\"\n", (int)c1.name.len, c1.name.ptr); failures++; }
    if (!EQ(next, "ok")) { fprintf(stderr, "FAIL: case1 — next_out = \"%.*s\", want \"ok\"\n", (int)next.len, next.ptr); failures++; }
    if (symtab.count != 1) { fprintf(stderr, "FAIL: case1 — symtab.count = %zu, want 1\n", symtab.count); failures++; }
    else {
        const KSymbol *sym = k_symtab_lookup(&symtab, (keel_slice_char){ 3, (char *)"MAX" });
        if (!sym) { fprintf(stderr, "FAIL: case1 — 'MAX' not found in symtab\n"); failures++; }
        else if (sym->kind != K_SYM_CONSTANT) { fprintf(stderr, "FAIL: case1 — kind = %d, want K_SYM_CONSTANT\n", sym->kind); failures++; }
    }

    /* case 2: no '=' at all — nothing registered, not a failure */
    KToken kw2 = first_token("constexpr unsigned long flag; ok2\n", &lexer);
    KConstexprDecl c2;
    bool ok2 = k_scan_decl_constexpr(&lexer, kw2, &symtab, &c2, &next, &pp);
    if (!ok2) { fprintf(stderr, "FAIL: case2 — returned false\n"); failures++; }
    if (c2.has_name) { fprintf(stderr, "FAIL: case2 — has_name = true, want false\n"); failures++; }
    if (!EQ(next, "ok2")) { fprintf(stderr, "FAIL: case2 — next_out = \"%.*s\", want \"ok2\"\n", (int)next.len, next.ptr); failures++; }
    if (symtab.count != 1) { fprintf(stderr, "FAIL: case2 — must not register anything, symtab.count = %zu, want 1\n", symtab.count); failures++; }

    /* case 3: '=' with no token at all before it — constexpr-name-missing */
    KToken kw3 = first_token("constexpr = 5; oops\n", &lexer);
    KConstexprDecl c3;
    bool ok3 = k_scan_decl_constexpr(&lexer, kw3, &symtab, &c3, &next, &pp);
    if (ok3) { fprintf(stderr, "FAIL: case3 — returned true, want false\n"); failures++; }
    if (symtab.count != 1) { fprintf(stderr, "FAIL: case3 — must not register anything, symtab.count = %zu, want 1\n", symtab.count); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
