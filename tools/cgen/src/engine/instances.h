#ifndef CGEN_INSTANCES_H
#define CGEN_INSTANCES_H
#include "engine/ast.h"
#define K_INSTANCE_NAME_CAP 256 /* 255 characters plus terminating zero */
/* Direct, concrete uses only; transitive generic instantiation is a later pass. */
typedef struct KInstanceUse {
    size_t first, end;
    char symbol[K_INSTANCE_NAME_CAP];
    size_t symbol_len;
} KInstanceUse;
bool k_collect_instances(KAst *, KInstanceUse *, size_t, KDiagnosticSink *);
#endif
