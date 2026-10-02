#ifndef CGEN_ISLANDS_H
#define CGEN_ISLANDS_H
#include "engine/ast.h"
/* Pass 3 (parser-design §3), stage 4a: the islands of kind type, name, call,
   ref and implicit-init, in signatures and bodies of the file's
   own declarations. The caller supplies ast->islands and ast->island_text;
   this fills them, sorted by anchor. A generic module is a template: it has
   no concrete islands, as it has no concrete instances. */
bool k_collect_islands(KAst *, KDiagnosticSink *);
#endif
