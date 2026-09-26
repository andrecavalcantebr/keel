/* Minimal oracle for the m2-parser-struct-decl task. Expected values are
 * hand-derived from the grammar, not from a solving implementation
 * (tools/harness/README.md, "O oráculo não pode ser resolvido escrevendo
 * a resposta"). Not model-generated.
 *
 * Trace for "struct Person { i32 age; }; ok" ('struct' already consumed):
 *   tok = k_lexer_next()        -> "Person" (IDENT) -> tag_name = "Person"
 *   tok = k_lexer_next()        -> "{"
 *   k_scan_braced_opaque(tok=...) reads "i32","age",";" as content, then
 *     "}" closes it (depth 0) -> body = "i32 age;", its own next_out = ";"
 *     (the decl-struct's own terminator, not yet advanced past)
 *   *next_out = k_lexer_next()  -> "ok"
 *
 * Trace for "struct { i32 x; }; ok2" (anonymous):
 *   tok = k_lexer_next()        -> "{" (not an IDENT) -> tag_name.len == 0
 *   k_scan_braced_opaque(tok=...) -> body = "i32 x;", next_out = ";"
 *   *next_out = k_lexer_next()  -> "ok2"
 *   nothing registered (no tag name)
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

    /* named */
    KToken kw = first_token("struct Person { i32 age; }; ok\n", &lexer);
    KStructDecl sd;
    bool ok1 = k_scan_struct_decl(&lexer, kw, &symtab, &sd, &next, &pp);
    if (!ok1) { fprintf(stderr, "FAIL: named — returned false\n"); failures++; }
    if (sd.is_union) { fprintf(stderr, "FAIL: named — is_union = true\n"); failures++; }
    if (!EQ(sd.tag_name, "Person")) { fprintf(stderr, "FAIL: named — tag_name = \"%.*s\"\n", (int)sd.tag_name.len, sd.tag_name.ptr); failures++; }
    if (!EQ(sd.body, "i32 age;")) { fprintf(stderr, "FAIL: named — body = \"%.*s\"\n", (int)sd.body.len, sd.body.ptr); failures++; }
    if (!EQ(next, "ok")) { fprintf(stderr, "FAIL: named — next_out = \"%.*s\", want \"ok\"\n", (int)next.len, next.ptr); failures++; }
    if (symtab.count != 1) { fprintf(stderr, "FAIL: symtab.count = %zu, want 1\n", symtab.count); failures++; }
    else {
        const KSymbol *sym = k_symtab_lookup(&symtab, (keel_slice_char){ 6, (char *)"Person" });
        if (!sym) { fprintf(stderr, "FAIL: 'Person' not found in symtab\n"); failures++; }
        else if (sym->kind != K_SYM_TYPE) { fprintf(stderr, "FAIL: kind = %d, want K_SYM_TYPE\n", sym->kind); failures++; }
    }

    /* anonymous */
    KToken kw2 = first_token("struct { i32 x; }; ok2\n", &lexer);
    KStructDecl sd2;
    bool ok2 = k_scan_struct_decl(&lexer, kw2, &symtab, &sd2, &next, &pp);
    if (!ok2) { fprintf(stderr, "FAIL: anonymous — returned false\n"); failures++; }
    if (sd2.tag_name.len != 0) { fprintf(stderr, "FAIL: anonymous — tag_name.len = %zu, want 0\n", sd2.tag_name.len); failures++; }
    if (!EQ(sd2.body, "i32 x;")) { fprintf(stderr, "FAIL: anonymous — body = \"%.*s\"\n", (int)sd2.body.len, sd2.body.ptr); failures++; }
    if (!EQ(next, "ok2")) { fprintf(stderr, "FAIL: anonymous — next_out = \"%.*s\", want \"ok2\"\n", (int)next.len, next.ptr); failures++; }
    if (symtab.count != 1) { fprintf(stderr, "FAIL: anonymous must not register anything — symtab.count = %zu, want 1\n", symtab.count); failures++; }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
