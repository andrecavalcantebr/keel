/* keel/keel_tagged_ag_Cycle_void.h — generated from keel/tagged.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_TAGGED_AG_CYCLE_VOID_H
#define KEEL_KEEL_TAGGED_AG_CYCLE_VOID_H
#include "keel/keel_tagged_ag_Cycle_void.type.h"

static inline i32  keel_tagged_ag_Cycle_void_tag (const keel_tagged_ag_Cycle_void *t);
static inline void keel_tagged_ag_Cycle_void_mark(keel_tagged_ag_Cycle_void *t, i32 e);

static inline i32  keel_tagged_ag_Cycle_void_tag (const keel_tagged_ag_Cycle_void *t) { return t->tag; }
static inline void keel_tagged_ag_Cycle_void_mark(keel_tagged_ag_Cycle_void *t, i32 e) { t->tag = e; }
#endif /* KEEL_KEEL_TAGGED_AG_CYCLE_VOID_H */
