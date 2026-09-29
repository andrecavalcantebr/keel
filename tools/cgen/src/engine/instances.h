#ifndef CGEN_INSTANCES_H
#define CGEN_INSTANCES_H
#include "engine/ast.h"
bool k_collect_instances(KAst *, KDiagnosticSink *);
/* The C symbol of a concrete type as written (`outcome i32` → keel_outcome_i32),
   into out[0..cap). SIZE_MAX if it is not a concrete use or does not fit. */
size_t k_spec_symbol(keel_slice_char text, const KSymbolTable *symbols, char *out, size_t cap);
#endif
