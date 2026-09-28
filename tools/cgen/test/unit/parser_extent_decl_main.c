/* Oracle for k_scan_extent_decl (keel-spec §2.2, §4.11). Expected values
 * are hand-derived from the grammar, not from running a solving
 * implementation (tools/harness/README.md, "O oráculo não pode ser
 * resolvido escrevendo a resposta"). Not model-generated.
 *
 *   decl-extent ::= 'extent' 'struct' IDENT extent-dim { extent-dim }
 *                   '{' { extent-field } '}' ';'
 *   extent-dim  ::= '[' IDENT ',' ( qualified-name | NUM ) ']'
 *
 * `next_out` is asserted in every case: it is the field that has broken
 * twice already in this parser, both times by landing inside the
 * declaration instead of after it.
 */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
/* len 0 short-circuits: a wrong implementation may leave .ptr NULL, and
   memcmp(NULL, ..., 0) is undefined — it must report FAIL, not abort,
   because the FAIL text is what feeds the next attempt. */
#define EQ(s, want) (strlen(want) == (s).len && \
                     ((s).len == 0 || memcmp((s).ptr, want, (s).len) == 0))

/* `want_dims` is the dimensions joined as "binder=cap|binder=cap". */
static void ok_case(const char *label, const char *src, const char *want_name,
                    const char *want_dims, const char *want_body,
                    const char *want_next) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, source, NULL);
    KToken kw = k_lexer_next(&lexer, &pp);   /* 'extent' */

    KSymbol storage[4];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 4);

    KExtentDecl ed;
    KToken next;
    if (!k_scan_extent_decl(&lexer, kw, &symtab, &ed, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned false\n", label);
        failures++;
        return;
    }
    if (!EQ(ed.name, want_name)) {
        fprintf(stderr, "FAIL: %s — name = \"%.*s\", want \"%s\"\n",
                label, (int)ed.name.len, ed.name.ptr, want_name);
        failures++;
    }

    char dims[160];
    size_t at = 0;
    for (size_t i = 0; i < ed.dim_count; i++) {
        if (i) dims[at++] = '|';
        if (ed.dim_names[i].ptr == NULL || ed.dim_caps[i].ptr == NULL) {
            memcpy(dims + at, "<null>", 6); at += 6; dims[at] = '\0';
            continue;
        }
        memcpy(dims + at, ed.dim_names[i].ptr, ed.dim_names[i].len);
        at += ed.dim_names[i].len;
        dims[at++] = '=';
        memcpy(dims + at, ed.dim_caps[i].ptr, ed.dim_caps[i].len);
        at += ed.dim_caps[i].len;
    }
    dims[at] = '\0';
    if (strcmp(dims, want_dims) != 0) {
        fprintf(stderr, "FAIL: %s — dims = \"%s\", want \"%s\"\n", label, dims, want_dims);
        failures++;
    }
    if (!EQ(ed.body, want_body)) {
        fprintf(stderr, "FAIL: %s — body = \"%.*s\", want \"%s\"\n",
                label, (int)ed.body.len, ed.body.ptr, want_body);
        failures++;
    }
    if (!EQ(next, want_next)) {
        fprintf(stderr, "FAIL: %s — next_out = \"%.*s\", want \"%s\"\n",
                label, (int)next.len, next.ptr, want_next);
        failures++;
    }
    if (symtab.len != 1 || storage[0].kind != K_SYM_TYPE ||
        !k_symtab_same_name(storage[0].name, ed.name)) {
        fprintf(stderr, "FAIL: %s — the name is not in the symtab as K_SYM_TYPE\n", label);
        failures++;
    }
}

static void reject_case(const char *label, const char *src) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, source, NULL);
    KToken kw = k_lexer_next(&lexer, &pp);
    KSymbol storage[4];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 4);
    KExtentDecl ed;
    KToken next;
    if (k_scan_extent_decl(&lexer, kw, &symtab, &ed, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned true, want false\n", label);
        failures++;
    }
}

int main(void) {
    /* The worked example of the task card, asserted field by field. */
    ok_case("one dim", "extent struct pos [len, cap] { bool active; } ; REST",
            "pos", "len=cap", "bool active;", "REST");

    /* golden/cases/020-extent/app/pos.k, the three real shapes */
    ok_case("num caps", "extent struct img [h, 2] [w, 3] { u8 r; } ; REST",
            "img", "h=2|w=3", "u8 r;", "REST");
    ok_case("two dims", "extent struct grid [rows, rcap] [cols, ccap] { i32 *v; } ; REST",
            "grid", "rows=rcap|cols=ccap", "i32 *v;", "REST");

    /* a capacity that is a qualified name, not a bare IDENT */
    ok_case("qualified", "extent struct g [r, m.MAX] { i32 v; } ; REST",
            "g", "r=m.MAX", "i32 v;", "REST");

    /* an empty body keeps its boundary pointer (lexer-design §7) */
    ok_case("empty body", "extent struct e [n, c] {} ; REST",
            "e", "n=c", "", "REST");

    /* the body is opaque: an inner '{' must not end it early */
    ok_case("nested body", "extent struct n [a, b] { struct { int q; } s; } ; REST",
            "n", "a=b", "struct { int q; } s;", "REST");

    reject_case("no struct",    "extent pos [a, b] { } ; REST");
    reject_case("no dimension", "extent struct pos { } ; REST");
    reject_case("no semicolon", "extent struct pos [a, b] { } REST");
    reject_case("truncated",    "extent struct pos [a, b");

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
