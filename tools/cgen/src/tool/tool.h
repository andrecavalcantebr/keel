#ifndef CGEN_TOOL_H
#define CGEN_TOOL_H
#include "engine/ast.h"
#include "tool/memory.h"
void cgen_loader_init(void *, keel_arena *, const char *const *, int, KDiagnosticSink *, KLoader *);
size_t cgen_loader_size(void);
void cgen_loader_destroy(void *);
KLoadResult cgen_load(void *, keel_slice_char, KModule **);
KLoadResult cgen_load_path(void *, const char *, const char *, KModule **);
/* a module whose source is `text` (the --instance invocation, cgen design §2.8);
   `label` names it in diagnostics */
KLoadResult cgen_load_text(void *, const char *label, const char *name, keel_slice_char text, KModule **);
void cgen_loader_report(void *, const KDiagnosticSink *);
bool cgen_path_normalize(const char *, char *, size_t);
bool cgen_module_name_of(const char *, const char *, char *, size_t);
bool cgen_module_path(const char *, size_t, const char *, char *, size_t);
#endif
