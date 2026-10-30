#ifndef CGEN_ISLANDS_H
#define CGEN_ISLANDS_H
#include "engine/ast.h"
/* Pass 3 (parser-design §3), stages 4a to 4h: the islands of the file's own
   declarations, in signatures and bodies, and of its `extent` structs. The caller supplies ast->islands and ast->island_text;
   this fills them, sorted by anchor. A generic module is a template: it has
   no concrete islands, as it has no concrete instances. */
bool k_collect_islands(KAst *, KDiagnosticSink *);

/* The islands of one instance of the generic module `generic` (codegen design §7):
   the functions that name a binder or the module's modifier, walked as the
   instance reads them — a binder is its argument, the modifier written bare is
   the instance. `self` is the generic's module; `symbol` the instance's C symbol;
   `arg` the C symbols of the type arguments, in the order of the `module` line.
   Fills ast->islands and ast->island_text as k_collect_islands does; the caller
   empties them before the next instance. Diagnostics go to `diag`, which the
   caller may drop: the generic was checked as a template. */
typedef struct {
    const KModule *self;
    keel_slice_char symbol, modifier;
    size_t argc;
    keel_slice_char arg[8];
    /* a function over a protocol (backend §5.19): set to its name, the walk reads
       the functions of that name in `generic` over the concrete type above —
       `self` is then the concrete type's module, `symbol` its own symbol */
    keel_slice_char proto_fn;
} KIslandInstance;
bool k_collect_instance_islands(KAst *generic, const KIslandInstance *inst, KDiagnosticSink *diag);
#endif
