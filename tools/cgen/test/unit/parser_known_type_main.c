/* Oracle for k_scan_known_type (keel-spec §2.2). Expected values are
 * hand-derived from the grammar, not from running a solving
 * implementation. Not model-generated.
 *
 *   known-type  ::= modifier argument { argument } | named-type
 *   modifier    ::= qualified-name [ '(' dim-value { ',' dim-value } ')' ]
 *   argument    ::= { qual-arg } ( known-type | tagged-type | base-type
 *                                | 'void' ) { qual-arg }
 *
 * The cases that matter are the ones where an argument is more than one
 * token. Reading one token per argument — what this function did until
 * 2026-09-27 — took `const` for the argument of `slice const char` and
 * left next_out on `char`, and still returned true.
 */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && \
                     ((s).len == 0 || memcmp((s).ptr, want, (s).len) == 0))

static void fill(KSymbolTable *t, KSymbol *storage, size_t cap) {
    k_symtab_init(t, storage, cap);
    k_symtab_insert(t, (keel_slice_char){ 5, (char *)"slice" },   K_SYM_MODIFIER, 1);
    k_symtab_insert(t, (keel_slice_char){ 6, (char *)"buffer" },  K_SYM_MODIFIER, 1);
    k_symtab_insert(t, (keel_slice_char){ 6, (char *)"tagged" },  K_SYM_MODIFIER, 2);
    k_symtab_insert(t, (keel_slice_char){ 5, (char *)"arena" },   K_SYM_TYPE, 0);
    k_symtab_insert(t, (keel_slice_char){ 3, (char *)"i32" },     K_SYM_TYPE, 0);
    k_symtab_insert(t, (keel_slice_char){ 5, (char *)"Cycle" },   K_SYM_TAGS, 0);
}

/* `want_args` and `want_dims` are the spans joined with '|'. */
static void modifier_case(const char *label, const char *src, const char *want_name,
                          const char *want_dims, const char *want_args,
                          const char *want_text, const char *want_next) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, source, NULL);
    KToken first = k_lexer_next(&lexer, &pp);
    KSymbol storage[8];
    KSymbolTable symtab;
    fill(&symtab, storage, 8);

    KSpecifier sp;
    KToken next = { 0, NULL };
    if (!k_scan_known_type(&lexer, first, &symtab, &sp, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned false\n", label);
        failures++;
        return;
    }
    if (sp.kind != K_SPEC_MODIFIER) {
        fprintf(stderr, "FAIL: %s — kind = %d, want K_SPEC_MODIFIER\n", label, sp.kind);
        failures++;
        return;
    }
    if (!EQ(sp.modifier_name, want_name)) {
        fprintf(stderr, "FAIL: %s — modifier = \"%.*s\", want \"%s\"\n",
                label, (int)sp.modifier_name.len, sp.modifier_name.ptr, want_name);
        failures++;
    }

    char buf[200];
    size_t at = 0;
    for (size_t i = 0; i < sp.dim_count; i++) {
        if (i) buf[at++] = '|';
        memcpy(buf + at, sp.dims[i].ptr, sp.dims[i].len); at += sp.dims[i].len;
    }
    buf[at] = '\0';
    if (strcmp(buf, want_dims) != 0) {
        fprintf(stderr, "FAIL: %s — dims = \"%s\", want \"%s\"\n", label, buf, want_dims);
        failures++;
    }
    at = 0;
    for (size_t i = 0; i < sp.arg_count; i++) {
        if (i) buf[at++] = '|';
        memcpy(buf + at, sp.args[i].ptr, sp.args[i].len); at += sp.args[i].len;
    }
    buf[at] = '\0';
    if (strcmp(buf, want_args) != 0) {
        fprintf(stderr, "FAIL: %s — args = \"%s\", want \"%s\"\n", label, buf, want_args);
        failures++;
    }
    if (!EQ(sp.text, want_text)) {
        fprintf(stderr, "FAIL: %s — text = \"%.*s\", want \"%s\"\n",
                label, (int)sp.text.len, sp.text.ptr, want_text);
        failures++;
    }
    if (!EQ(next, want_next)) {
        fprintf(stderr, "FAIL: %s — next_out = \"%.*s\", want \"%s\"\n",
                label, (int)next.len, next.ptr, want_next);
        failures++;
    }
}

static void reject_case(const char *label, const char *src) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, source, NULL);
    KToken first = k_lexer_next(&lexer, &pp);
    KSymbol storage[8];
    KSymbolTable symtab;
    fill(&symtab, storage, 8);
    KSpecifier sp;
    KToken next = { 0, NULL };
    if (k_scan_known_type(&lexer, first, &symtab, &sp, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned true, want false\n", label);
        failures++;
    }
}

int main(void) {
    /* one token per argument — the case that already worked */
    modifier_case("simple", "slice i32 xs;", "slice", "", "i32", "slice i32", "xs");

    /* an argument of several tokens: this is what used to break */
    modifier_case("leading const", "slice const char xs;",
                  "slice", "", "const char", "slice const char", "xs");
    modifier_case("trailing const", "slice char const xs;",
                  "slice", "", "char const", "slice char const", "xs");
    modifier_case("tagged-type arg", "slice struct Person p;",
                  "slice", "", "struct Person", "slice struct Person", "p");
    modifier_case("qualified tag", "slice struct a.b.Person p;",
                  "slice", "", "struct a.b.Person", "slice struct a.b.Person", "p");

    /* a whole known-type as the argument */
    modifier_case("nested modifier", "buffer slice i32 b;",
                  "buffer", "", "slice i32", "buffer slice i32", "b");
    modifier_case("nested qualified", "buffer slice const char b;",
                  "buffer", "", "slice const char", "buffer slice const char", "b");

    /* the parenthesized dim-value list */
    modifier_case("dim num", "buffer(16) i32 b;",
                  "buffer", "16", "i32", "buffer(16) i32", "b");
    modifier_case("dim named", "buffer(N) i32 b;",
                  "buffer", "N", "i32", "buffer(N) i32", "b");
    modifier_case("two dims", "buffer(R, C) i32 b;",
                  "buffer", "R|C", "i32", "buffer(R, C) i32", "b");

    /* arity 2, from the declaring module, not free repetition */
    modifier_case("arity two", "tagged Cycle void x;",
                  "tagged", "", "Cycle|void", "tagged Cycle void", "x");
    modifier_case("bool arg", "slice bool flags;", "slice", "", "bool", "slice bool", "flags");

    /* named-type, and the two non-specifier outcomes */
    {
        const char *src = "arena a;";
        keel_slice_char source = { strlen(src), (char *)src };
        KLexer lexer; TKPpKind pp;
        k_lexer_init(&lexer, source, NULL);
        KToken first = k_lexer_next(&lexer, &pp);
        KSymbol storage[8]; KSymbolTable symtab; fill(&symtab, storage, 8);
        KSpecifier sp; KToken next = { 0, NULL };
        if (!k_scan_known_type(&lexer, first, &symtab, &sp, &next, &pp) ||
            sp.kind != K_SPEC_NAMED_TYPE || !EQ(sp.type_name, "arena") ||
            !EQ(sp.text, "arena") || !EQ(next, "a")) {
            fprintf(stderr, "FAIL: named type — kind/name/text/next wrong\n");
            failures++;
        }
    }
    {
        const char *src = "unregistered rest";
        keel_slice_char source = { strlen(src), (char *)src };
        KLexer lexer; TKPpKind pp;
        k_lexer_init(&lexer, source, NULL);
        KToken first = k_lexer_next(&lexer, &pp);
        KSymbol storage[8]; KSymbolTable symtab; fill(&symtab, storage, 8);
        KSpecifier sp; KToken next = { 0, NULL };
        if (!k_scan_known_type(&lexer, first, &symtab, &sp, &next, &pp) ||
            sp.kind != K_SPEC_NONE) {
            fprintf(stderr, "FAIL: unregistered — must be K_SPEC_NONE and true\n");
            failures++;
        }
        if (next.len != 0) {
            fprintf(stderr, "FAIL: unregistered — next_out must be untouched\n");
            failures++;
        }
    }

    /* an argument that is in no symbol table is not an argument */
    reject_case("unknown arg", "slice zzz x;");
    reject_case("dim not a value", "buffer(*) i32 b;");
    reject_case("truncated dims", "buffer(16");
    reject_case("truncated arg", "slice const");

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
