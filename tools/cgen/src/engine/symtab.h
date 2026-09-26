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
#include "engine/lexer.h"

typedef enum {
    K_SYM_TYPE,        /* typedef, struct/union/enum nomeado, tipo de módulo */
    K_SYM_MODIFIER,    /* aridade e espécie de cada parâmetro                */
    K_SYM_FUNCTION,    /* retorno declarado e aridades                       */
    K_SYM_VARIABLE,
    K_SYM_CONSTANT,    /* constexpr, constante de enum                       */
    K_SYM_TAGS,        /* conjunto fechado e seus valores                    */
    K_SYM_MODULE,      /* alias ativo de import                              */
    K_SYM_EXTERN_C     /* nome de extern_c: só a distinção variável/função   */
} KSymKind;

typedef struct {
    keel_slice_char name;   /* the keel name, as spelled */
    KSymKind kind;
    int arity;   /* K_SYM_MODIFIER: the declaring module's own dim+tags+type
                    binder count (parser-design §4.3: "a assinatura do
                    módulo fixa a quantidade... de todos os seus
                    modificadores" — every modifier of a module shares its
                    arity). K_SYM_FUNCTION: parameter count. Unused (0) for
                    the other kinds so far — this grows with later etapas,
                    same way KModuleHeader grew its binders. */
} KSymbol;

/* [P2] Ordered by insertion, not by hash — the emission order is the
 * declaration order (codegen §6), and iteration here is that order. */
typedef struct {
    KSymbol *items;   /* caller's storage */
    size_t cap;
    size_t count;
} KSymbolTable;

static inline void k_symtab_init(KSymbolTable *t, KSymbol *storage, size_t cap) {
    t->items = storage;
    t->cap = storage != NULL ? cap : 0;
    t->count = 0;
}

/* Appends unconditionally — this layer does not check whether `name` is
 * already present; that is symbol-collision/redeclaration diagnosis, a
 * later concern (parser-design §7), not this data structure's job.
 * Returns false, without writing past items[cap-1], if the table is full. */
static inline bool k_symtab_insert(KSymbolTable *t, keel_slice_char name, KSymKind kind, int arity) {
    if (t->count >= t->cap) return false;
    t->items[t->count++] = (KSymbol){ .name = name, .kind = kind, .arity = arity };
    return true;
}

static inline bool k_symtab_same_name(keel_slice_char a, keel_slice_char b) {
    return a.len == b.len && (a.len == 0 || memcmp(a.ptr, b.ptr, a.len) == 0);
}

/* Linear search by spelling (parser-design §2.1: the search is by grafia,
 * not by hash — lesson 3 of tools/harness/README.md applies here too, and
 * a keel module's own symbol count never approaches where it would
 * matter). Returns the first match in insertion order, or NULL. */
static inline const KSymbol *k_symtab_lookup(const KSymbolTable *t, keel_slice_char name) {
    for (size_t i = 0; i < t->count; i++) {
        if (k_symtab_same_name(t->items[i].name, name)) return &t->items[i];
    }
    return NULL;
}

#endif /* CGEN_ENGINE_SYMTAB_H */
