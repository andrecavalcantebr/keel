/* Oracle for k_scan_decl_array (keel-spec §2.2, §4.2). Expected values
 * are hand-derived from the grammar, not from running a solving
 * implementation. Not model-generated.
 *
 *   decl-array   ::= { spec-c } 'array' argument decl-array-1
 *                    { ',' decl-array-1 } ';'
 *   decl-array-1 ::= { '*' } IDENT dimensions [ '=' <opaque> ]
 *   dimensions   ::= '[' [ <opaque> { ',' <opaque> } ] ']'
 *                  | '[' [ <opaque> ] ']' { '[' <opaque> ']' }
 *
 * `next_out` is asserted in every case: it is the field that has broken
 * three times in this parser, always by stopping inside the declaration.
 */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && \
                     ((s).len == 0 || memcmp((s).ptr, want, (s).len) == 0))

static void fill(KSymbolTable *t, KSymbol *storage, size_t cap) {
    k_symtab_init(t, storage, cap);
    k_symtab_insert(t, (keel_slice_char){ 3, (char *)"i32" },    K_SYM_TYPE, 0);
    k_symtab_insert(t, (keel_slice_char){ 2, (char *)"u8" },     K_SYM_TYPE, 0);
    k_symtab_insert(t, (keel_slice_char){ 6, (char *)"Person" }, K_SYM_TYPE, 0);
    k_symtab_insert(t, (keel_slice_char){ 5, (char *)"slice" },  K_SYM_MODIFIER, 1);
}

/* `want_names` joins each name as "[*]name dims", separated by '|':
   "v [2,3,4]" for a plain name, "*p [3]" for one pointer. */
static void ok_case(const char *label, const char *src, const char *want_element,
                    const char *want_names, const char *want_next) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, source, NULL);
    KToken kw = k_lexer_next(&lexer, &pp);   /* 'array' */
    KSymbol storage[16];
    KSymbolTable symtab;
    fill(&symtab, storage, 16);

    KArrayDecl ad;
    KToken next;
    if (!k_scan_decl_array(&lexer, kw, &symtab, &ad, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned false\n", label);
        failures++;
        return;
    }
    if (!EQ(ad.element, want_element)) {
        fprintf(stderr, "FAIL: %s — element = \"%.*s\", want \"%s\"\n",
                label, (int)ad.element.len, ad.element.ptr, want_element);
        failures++;
    }

    char buf[240];
    size_t at = 0;
    for (size_t i = 0; i < ad.name_count; i++) {
        if (i) buf[at++] = '|';
        for (int k = 0; k < ad.pointer_depth[i]; k++) buf[at++] = '*';
        if (ad.names[i].ptr) {
            memcpy(buf + at, ad.names[i].ptr, ad.names[i].len); at += ad.names[i].len;
        }
        buf[at++] = ' ';
        if (ad.dims[i].ptr) {
            memcpy(buf + at, ad.dims[i].ptr, ad.dims[i].len); at += ad.dims[i].len;
        }
    }
    buf[at] = '\0';
    if (strcmp(buf, want_names) != 0) {
        fprintf(stderr, "FAIL: %s — names = \"%s\", want \"%s\"\n", label, buf, want_names);
        failures++;
    }
    if (!EQ(next, want_next)) {
        fprintf(stderr, "FAIL: %s — next_out = \"%.*s\", want \"%s\"\n",
                label, (int)next.len, next.ptr, want_next);
        failures++;
    }
    /* every name registered, and nothing else added beyond the 4 seeded */
    if (symtab.len != 4 + ad.name_count) {
        fprintf(stderr, "FAIL: %s — symtab.len = %zu, want %zu\n",
                label, symtab.len, 4 + ad.name_count);
        failures++;
    } else {
        for (size_t i = 0; i < ad.name_count; i++) {
            if (storage[4 + i].kind != K_SYM_VARIABLE ||
                !k_symtab_same_name(storage[4 + i].name, ad.names[i])) {
                fprintf(stderr, "FAIL: %s — name %zu not registered as a variable\n", label, i);
                failures++;
            }
        }
    }
}

static void reject_case(const char *label, const char *src) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, source, NULL);
    KToken kw = k_lexer_next(&lexer, &pp);
    KSymbol storage[16];
    KSymbolTable symtab;
    fill(&symtab, storage, 16);
    KArrayDecl ad;
    KToken next;
    if (k_scan_decl_array(&lexer, kw, &symtab, &ad, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned true, want false\n", label);
        failures++;
    }
}

int main(void) {
    /* the worked example of the task card */
    ok_case("comma rank", "array i32 v[2,3,4]; REST", "i32", "v [2,3,4]", "REST");

    /* golden/cases/013-sugar and 020-extent shapes */
    ok_case("bracket rank", "array i32 v[2][3]; REST",  "i32", "v [2][3]", "REST");
    ok_case("named dims",   "array u8 r[H, W]; REST",   "u8",  "r [H, W]", "REST");
    ok_case("module type",  "array Person people[4]; REST",
                                                        "Person", "people [4]", "REST");

    /* the element is a whole `argument`, not one token */
    ok_case("qualified el", "array const char *p[3]; REST",
                                                        "const char", "*p [3]", "REST");
    ok_case("nested el",    "array slice i32 rows[2]; REST",
                                                        "slice i32", "rows [2]", "REST");

    /* several names, and an initializer that must be skipped whole */
    ok_case("two names",    "array i32 a[2], b[3]; REST", "i32", "a [2]|b [3]", "REST");
    ok_case("initializer",  "array i32 v[4] = {0, 1}; REST", "i32", "v [4]", "REST");
    ok_case("init comma",   "array i32 a[2] = {1, 2}, b[3]; REST",
                                                        "i32", "a [2]|b [3]", "REST");

    /* an empty dimension is legal: `'[' [ <opaque> ] ']'` */
    ok_case("empty dims",   "array i32 v[]; REST",      "i32", "v []", "REST");

    reject_case("unknown element", "array zzz v[2]; REST");
    reject_case("no dimensions",   "array i32 v; REST");
    reject_case("no semicolon",    "array i32 v[2] REST");
    reject_case("truncated",       "array i32 v[2");

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
