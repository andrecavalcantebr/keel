/* engine/symtab.h — the symbol table (parser-design §2.1). engine/: no I/O,
 * no allocation — the caller gives the storage, same convention as
 * engine/diag.h's KDiagnosticSink.
 *
 * Every function here is `static inline`, not split into a .c: they are
 * tiny and called from all over the parser (a symtab lookup happens at
 * nearly every declaration), the same reason the generated base itself
 * keeps its own small per-instance functions inline in the header
 * (`keel_slice_i32_begin` and siblings, in gen/keel/keel_slice_i32.h) —
 * this is that same convention, not a special case for symtab. */
#ifndef CGEN_ENGINE_SYMTAB_H
#define CGEN_ENGINE_SYMTAB_H

#include <string.h>
#include "keel/keel_buffer_KSymbol.h"

/* [P2] Ordered by insertion, not by hash — the emission order is the
 * declaration order (codegen §6), and iteration here is that order. */
typedef keel_buffer_KSymbol KSymbolTable;

static inline void k_symtab_init(KSymbolTable *t, KSymbol *storage, size_t cap) {
    *t = keel_buffer_KSymbol_from(storage, cap);
}

/* Appends unconditionally — this layer does not check whether `name` is
 * already present; that is symbol-collision/redeclaration diagnosis, a
 * later concern (parser-design §7), not this data structure's job.
 * Returns false, without writing past the buffer capacity, if the table is full. */
static inline bool k_symtab_insert(KSymbolTable *t, keel_slice_char name, KSymKind kind, int arity) {
    return keel_buffer_KSymbol_push_1(t,
        (KSymbol){ .name=name, .kind=kind, .arity=arity }) != NULL;
}

static inline bool k_symtab_same_name(keel_slice_char a, keel_slice_char b) {
    return a.len == b.len && (a.len == 0 || memcmp(a.ptr, b.ptr, a.len) == 0);
}

/* Linear search by spelling (parser-design §2.1: the search is by grafia,
 * not by hash — lesson 3 of tools/harness/README.md applies here too, and
 * a keel module's own symbol count never approaches where it would
 * matter). Returns the first match in insertion order, or NULL. */
static inline const KSymbol *k_symtab_lookup(const KSymbolTable *t, keel_slice_char name) {
    for (size_t i = 0; i < t->len; i++) {
        if (k_symtab_same_name(keel_buffer_KSymbol_ptr(t, i)->name, name)) return keel_buffer_KSymbol_ptr(t, i);
    }
    return NULL;
}

#endif /* CGEN_ENGINE_SYMTAB_H */
