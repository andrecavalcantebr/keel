/* Minimal oracle for the m2-parser-tags-decl task. Expected values are
 * hand-derived from the grammar (keel-spec §2.2), not from running a
 * solving implementation (tools/harness/README.md, "O oráculo não pode
 * ser resolvido escrevendo a resposta"). Not model-generated.
 *
 * Trace for "tags Cycle [ST1, ST2, ST3]; ok" ('tags' already consumed):
 *   name = k_lexer_next()      -> "Cycle"
 *   [ = k_lexer_next()          -> "[" (discarded)
 *   first = k_lexer_next()      -> "ST1"
 *   k_scan_ident_list(first, ...) reads ",", "ST2", ",", "ST3", then "]"
 *     (not ",", so it stops there) -> items = [ST1, ST2, ST3], tok = "]"
 *   ; = k_lexer_next()          -> ";" (the lexer had only read up to "]",
 *     same "not yet at the terminator" situation as k_scan_import_c)
 *   next_out = k_lexer_next()   -> "ok"
 */
#include <stdio.h>
#include <string.h>
#include "engine/parser.h"

static int failures = 0;
#define EQ(s, want) (strlen(want) == (s).len && memcmp((s).ptr, want, (s).len) == 0)

int main(void) {
    keel_slice_char src = { 0 };
    const char *s = "tags Cycle [ST1, ST2, ST3]; ok";
    src.len = strlen(s); src.ptr = (char *)s;
    KLexer lexer; TKPpKind pp;
    k_lexer_init(&lexer, src, NULL);
    KToken kw = k_lexer_next(&lexer, &pp); /* "tags" */

    KSymbol storage[4];
    KSymbolTable symtab;
    k_symtab_init(&symtab, storage, 4);

    KTagsDecl td; KToken next; TKPpKind npp;
    bool inserted = k_scan_tags_decl(&lexer, kw, &symtab, &td, &next, &npp);

    if (!inserted) { fprintf(stderr, "FAIL: k_scan_tags_decl returned false\n"); failures++; }
    if (!EQ(td.name, "Cycle")) { fprintf(stderr, "FAIL: name = \"%.*s\"\n", (int)td.name.len, td.name.ptr); failures++; }
    if (td.item_count != 3) { fprintf(stderr, "FAIL: item_count = %zu, want 3\n", td.item_count); failures++; }
    else {
        if (!EQ(td.items[0], "ST1")) { fprintf(stderr, "FAIL: items[0] = \"%.*s\"\n", (int)td.items[0].len, td.items[0].ptr); failures++; }
        if (!EQ(td.items[1], "ST2")) { fprintf(stderr, "FAIL: items[1] = \"%.*s\"\n", (int)td.items[1].len, td.items[1].ptr); failures++; }
        if (!EQ(td.items[2], "ST3")) { fprintf(stderr, "FAIL: items[2] = \"%.*s\"\n", (int)td.items[2].len, td.items[2].ptr); failures++; }
    }
    if (!EQ(next, "ok")) { fprintf(stderr, "FAIL: next_out = \"%.*s\", want \"ok\"\n", (int)next.len, next.ptr); failures++; }

    if (symtab.count != 1) { fprintf(stderr, "FAIL: symtab.count = %zu, want 1\n", symtab.count); failures++; }
    else {
        const KSymbol *sym = k_symtab_lookup(&symtab, (keel_slice_char){ 5, (char *)"Cycle" });
        if (!sym) { fprintf(stderr, "FAIL: 'Cycle' not found in symtab\n"); failures++; }
        else if (sym->kind != K_SYM_TAGS) { fprintf(stderr, "FAIL: kind = %d, want K_SYM_TAGS\n", sym->kind); failures++; }
    }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
