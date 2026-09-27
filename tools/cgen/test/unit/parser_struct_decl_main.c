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

    /* keel-spec §2.2 (2026-09-27): the declarators after the '}' declare
       objects of the type. They used to be swallowed — this function read
       one token past the '}' assuming it was the ';'. */
    {
        struct { const char *label, *src, *tag, *names, *next; size_t syms; } cases[] = {
            { "one object",  "struct Foo { i32 x; } inst; ok3",  "Foo", "inst",  "ok3", 2 },
            { "two objects", "struct Foo { i32 x; } a, *b; ok4", "Foo", "a,*b",  "ok4", 3 },
            { "array object","struct Foo { i32 x; } v[4]; ok5",  "Foo", "v",     "ok5", 2 },
            { "no object",   "struct Foo { i32 x; }; ok6",       "Foo", "",      "ok6", 1 },
            { "anon object", "struct { i32 x; } only; ok7",      "",    "only",  "ok7", 1 },
        };
        for (size_t c = 0; c < sizeof cases / sizeof *cases; c++) {
            const char *src = cases[c].src;
            keel_slice_char source = { strlen(src), (char *)src };
            KLexer lx; TKPpKind p2;
            k_lexer_init(&lx, source, NULL);
            KToken kw = k_lexer_next(&lx, &p2);
            KSymbol st[8]; KSymbolTable t; k_symtab_init(&t, st, 8);
            KStructDecl sd; KToken nx;
            if (!k_scan_struct_decl(&lx, kw, &t, &sd, &nx, &p2)) {
                fprintf(stderr, "FAIL: %s — returned false\n", cases[c].label);
                failures++;
                continue;
            }
            if (!EQ(sd.tag_name, cases[c].tag)) {
                fprintf(stderr, "FAIL: %s — tag \"%.*s\", want \"%s\"\n",
                        cases[c].label, (int)sd.tag_name.len, sd.tag_name.ptr, cases[c].tag);
                failures++;
            }
            char got[80]; size_t at = 0;
            for (size_t i = 0; i < sd.name_count; i++) {
                if (i) got[at++] = ',';
                for (int k = 0; k < sd.pointer_depth[i]; k++) got[at++] = '*';
                memcpy(got + at, sd.names[i].ptr, sd.names[i].len); at += sd.names[i].len;
            }
            got[at] = '\0';
            if (strcmp(got, cases[c].names) != 0) {
                fprintf(stderr, "FAIL: %s — objects \"%s\", want \"%s\"\n",
                        cases[c].label, got, cases[c].names);
                failures++;
            }
            if (!EQ(nx, cases[c].next)) {
                fprintf(stderr, "FAIL: %s — next \"%.*s\", want \"%s\"\n",
                        cases[c].label, (int)nx.len, nx.ptr, cases[c].next);
                failures++;
            }
            if (t.count != cases[c].syms) {
                fprintf(stderr, "FAIL: %s — symtab.count = %zu, want %zu\n",
                        cases[c].label, t.count, cases[c].syms);
                failures++;
            }
        }
    }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
