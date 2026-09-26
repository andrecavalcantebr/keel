/* Minimal oracle for the m2-parser-modifier-decl task. Not model-generated. */
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

    /* modifier buffer byref { size_t cap, len; T *ptr; } ok */
    KToken kw = first_token("modifier buffer byref { size_t cap, len; T *ptr; } ok\n", &lexer);
    KModifierDecl md;
    bool inserted = k_scan_modifier_decl(&lexer, kw, 1, &symtab, &md, &next, &pp);

    if (!inserted) { fprintf(stderr, "FAIL: k_scan_modifier_decl returned false\n"); failures++; }
    if (!EQ(md.name, "buffer")) { fprintf(stderr, "FAIL: name = \"%.*s\"\n", (int)md.name.len, md.name.ptr); failures++; }
    if (!md.byref) { fprintf(stderr, "FAIL: byref = false\n"); failures++; }
    if (!EQ(md.body, "size_t cap, len; T *ptr;")) { fprintf(stderr, "FAIL: body = \"%.*s\"\n", (int)md.body.len, md.body.ptr); failures++; }
    if (!EQ(next, "ok")) { fprintf(stderr, "FAIL: next_out = \"%.*s\", want \"ok\"\n", (int)next.len, next.ptr); failures++; }

    if (symtab.count != 1) { fprintf(stderr, "FAIL: symtab.count = %zu, want 1\n", symtab.count); failures++; }
    else {
        const KSymbol *sym = k_symtab_lookup(&symtab, (keel_slice_char){ 6, (char *)"buffer" });
        if (!sym) { fprintf(stderr, "FAIL: 'buffer' not found in symtab\n"); failures++; }
        else {
            if (sym->kind != K_SYM_MODIFIER) { fprintf(stderr, "FAIL: kind = %d, want K_SYM_MODIFIER\n", sym->kind); failures++; }
            if (sym->arity != 1) { fprintf(stderr, "FAIL: arity = %d, want 1\n", sym->arity); failures++; }
        }
    }

    if (failures == 0) { puts("ok"); return 0; }
    fprintf(stderr, "%d failure(s)\n", failures);
    return 1;
}
