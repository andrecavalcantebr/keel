#ifndef CGEN_ISLANDS_H
#define CGEN_ISLANDS_H
#include "engine/ast.h"
/* Pass 3 (parser-design §3), stages 4a to 4h: the islands of the file's own
   declarations, in signatures and bodies, and of its `extent` structs. The caller supplies ast->islands and ast->island_text;
   this fills them, sorted by anchor. A generic module is a template: it has
   no concrete islands, as it has no concrete instances. */
bool k_collect_islands(KAst *, KDiagnosticSink *);
#endif
