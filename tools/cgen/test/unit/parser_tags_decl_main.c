/* Oracle for k_scan_tags_decl (keel-spec §2.2, §4.9). Expected values are
 * hand-derived from the grammar, not from running a solving
 * implementation (tools/harness/README.md, "O oráculo não pode ser
 * resolvido escrevendo a resposta"). Not model-generated.
 *
 *   tags-list ::= '[' tag-item { ',' tag-item } ']'
 *   tag-item  ::= IDENT [ '=' tag-value ]
 *   tag-value ::= [ '-' ] ( NUM | qualified-name )
 *
 * The valued forms are the ones the first version got wrong: it scanned
 * the list with k_scan_ident_list, which stops at the first '=', and
 * then assumed the two tokens after it were ']' and ';'. On
 * `[SUCCESS = -1, ...]` that left item_count at 1 and next_out pointing
 * at the '1' of '-1', inside the brackets.
 */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && memcmp((s).ptr, want, (s).len) == 0)

/* `items` and `values` are comma-joined; an item with no value writes an
 * empty field, so "-1,,2" means the middle item has none. */
static void ok_case(const char *label, const char *src, const char *want_name,
                    const char *want_items, const char *want_values,
                    bool want_has, bool want_all, const char *want_next) {
    keel_slice_char source = { strlen(src), (char *)src };
    KLexer lexer;
    TKPpKind pp;
    k_lexer_init(&lexer, source, NULL);
    KToken kw = k_lexer_next(&lexer, &pp);   /* 'tags' */

    KSymbol storage[4];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 4);

    KTagsDecl td;
    KToken next;
    if (!k_scan_tags_decl(&lexer, kw, &symtab, &td, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned false\n", label);
        failures++;
        return;
    }
    if (!EQ(td.name, want_name)) {
        fprintf(stderr, "FAIL: %s — name = \"%.*s\", want \"%s\"\n",
                label, (int)td.name.len, td.name.ptr, want_name);
        failures++;
    }

    char items[160], values[160];
    size_t ai = 0, av = 0;
    for (size_t i = 0; i < td.item_count; i++) {
        if (i) { items[ai++] = ','; values[av++] = ','; }
        memcpy(items + ai, td.items[i].ptr, td.items[i].len);  ai += td.items[i].len;
        if (td.values[i].len) {
            memcpy(values + av, td.values[i].ptr, td.values[i].len);
            av += td.values[i].len;
        }
    }
    items[ai] = '\0'; values[av] = '\0';
    if (strcmp(items, want_items) != 0) {
        fprintf(stderr, "FAIL: %s — items = \"%s\", want \"%s\"\n", label, items, want_items);
        failures++;
    }
    if (strcmp(values, want_values) != 0) {
        fprintf(stderr, "FAIL: %s — values = \"%s\", want \"%s\"\n", label, values, want_values);
        failures++;
    }
    if (td.has_values != want_has || td.all_values != want_all) {
        fprintf(stderr, "FAIL: %s — has/all = %d/%d, want %d/%d\n",
                label, (int)td.has_values, (int)td.all_values, (int)want_has, (int)want_all);
        failures++;
    }
    if (!EQ(next, want_next)) {
        fprintf(stderr, "FAIL: %s — next_out = \"%.*s\", want \"%s\"\n",
                label, (int)next.len, next.ptr, want_next);
        failures++;
    }
    if (symtab.count != 1 || storage[0].kind != K_SYM_TAGS ||
        !k_symtab_same_name(storage[0].name, td.name)) {
        fprintf(stderr, "FAIL: %s — the set name is not in the symtab as K_SYM_TAGS\n", label);
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
    KTagsDecl td;
    KToken next;
    if (k_scan_tags_decl(&lexer, kw, &symtab, &td, &next, &pp)) {
        fprintf(stderr, "FAIL: %s — returned true, want false\n", label);
        failures++;
    }
}

int main(void) {
    /*      label        source                                  name    items          values    has   all   next */
    ok_case("no values", "tags Cycle [ST1, ST2, ST3]; ok",        "Cycle", "ST1,ST2,ST3", ",,",     false, false, "ok");
    ok_case("one item",  "tags One [ONLY]; ok",                   "One",   "ONLY",        "",       false, false, "ok");

    /* base/keel/corot.k:12 — the shape that used to derail the scan */
    ok_case("values",    "tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1]; ok",
                                                                  "Status", "SUCCESS,ONGOING,FAILED", "-1,0,1", true, true, "ok");
    ok_case("named val", "tags T [A = keel.LIMIT, B = OTHER]; ok", "T",     "A,B",         "keel.LIMIT,OTHER", true, true, "ok");
    ok_case("neg named", "tags T [A = -LIMIT]; ok",                "T",     "A",           "-LIMIT", true, true, "ok");

    /* §4.9 forbids mixing, but the production admits it: the recognizer
       records it, and `partial-tag-values` is a later pass's call. */
    ok_case("mixed",     "tags T [A = 1, B, C = 3]; ok",           "T",     "A,B,C",       "1,,3",   true, false, "ok");

    reject_case("no bracket", "tags T ST1, ST2; ok");
    reject_case("no semicolon", "tags T [A, B] ok");
    reject_case("value missing", "tags T [A = ]; ok");
    reject_case("truncated", "tags T [A, B");

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
