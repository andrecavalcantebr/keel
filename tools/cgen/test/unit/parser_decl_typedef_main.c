/* Oracle for k_scan_decl_typedef (keel-spec §2.2). Expected values are
 * hand-derived from the grammar, not from a solving implementation.
 * Not model-generated.
 *
 * Every case whose name is not the last IDENT before the ';' is here:
 * those are exactly the ones the old ast.c heuristic got wrong. */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && memcmp((s).ptr, want, (s).len) == 0)

/* `src` starts at the 'typedef'. `want` is the comma-joined list of the
 * names the declaration must register, in order. */
static void ok_case(const char *label, const char *src, const char *want,
                    const char *want_next) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, source, NULL);
    KToken kw = k_lexer_next(&lexer, &pp);

    KSymbol storage[16];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 16);

    KTypedefDecl d;
    KToken next;
    if (!k_scan_decl_typedef(&lexer, kw, &symtab, &d, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned false\n", label);
        failures++;
        return;
    }

    char got[128];
    size_t at = 0;
    for (size_t i = 0; i < d.name_count; i++) {
        if (i) got[at++] = ',';
        memcpy(got + at, d.names[i].ptr, d.names[i].len);
        at += d.names[i].len;
    }
    got[at] = '\0';
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL: %s — names = \"%s\", want \"%s\"\n", label, got, want);
        failures++;
    }

    /* every name registered as a type, in the same order */
    if (symtab.count != d.name_count) {
        fprintf(stderr, "FAIL: %s — symtab has %zu symbols, want %zu\n",
                label, symtab.count, d.name_count);
        failures++;
    } else {
        for (size_t i = 0; i < d.name_count; i++) {
            if (storage[i].kind != K_SYM_TYPE ||
                !k_symtab_same_name(storage[i].name, d.names[i])) {
                fprintf(stderr, "FAIL: %s — symtab[%zu] is not the type \"%.*s\"\n",
                        label, i, (int)d.names[i].len, d.names[i].ptr);
                failures++;
            }
        }
    }

    if (!EQ(next, want_next)) {
        fprintf(stderr, "FAIL: %s — next_out = \"%.*s\", want \"%s\"\n",
                label, (int)next.len, next.ptr, want_next);
        failures++;
    }
}

int main(void) {
    /* the plain shapes */
    ok_case("simple",      "typedef int i32; rest\n",            "i32",  "rest");
    ok_case("two words",   "typedef unsigned long ul; rest\n",   "ul",   "rest");
    ok_case("tagged",      "typedef struct Foo Bar; rest\n",     "Bar",  "rest");
    ok_case("pointer",     "typedef int *pint; rest\n",          "pint", "rest");
    ok_case("qual-c",      "typedef int * const * p; rest\n",    "p",    "rest");

    /* the name is before the brackets, not inside them */
    ok_case("array named", "typedef int Vec[TAM]; rest\n",       "Vec",  "rest");
    ok_case("array 2d",    "typedef int Mat[TAM][OUTRO]; rest\n","Mat",  "rest");

    /* the name is before the parameter group, not inside it */
    ok_case("fn pointer",  "typedef int (*fp)(void); rest\n",    "fp",   "rest");
    ok_case("fn params",   "typedef int (*cmp)(void *a, void *b); rest\n",
                                                                 "cmp",  "rest");
    ok_case("fn twice",    "typedef int (*(*f)(void))(int); rest\n", "f", "rest");

    /* the comma list (keel-spec §2.2, 2026-09-27) */
    ok_case("list",        "typedef int a[4], *b; rest\n",       "a,b",  "rest");
    ok_case("list three",  "typedef int x, *y, z[N]; rest\n",    "x,y,z","rest");
    ok_case("struct list", "typedef struct Bar { int y; } Bar, *BarRef; rest\n",
                                                                 "Bar,BarRef", "rest");
    /* a struct body whose fields would confuse a token scan */
    ok_case("struct body", "typedef struct { int a[N]; int (*f)(void); } S; rest\n",
                                                                 "S",    "rest");

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
