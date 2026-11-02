/* keel/keel_tagged_app_mm_Cycle_void.h — generated from keel/tagged.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_TAGGED_APP_MM_CYCLE_VOID_H
#define KEEL_KEEL_TAGGED_APP_MM_CYCLE_VOID_H

#include "keel/keel_tagged_app_mm_Cycle_void.type.h"
#include "app/app_mm.type.h"

#line 18 "keel/tagged.k"
static inline app_mm_Cycle    keel_tagged_app_mm_Cycle_void_tag  (const keel_tagged_app_mm_Cycle_void *t);
#line 21 "keel/tagged.k"
static inline void keel_tagged_app_mm_Cycle_void_mark (keel_tagged_app_mm_Cycle_void *t, app_mm_Cycle e);

#line 18 "keel/tagged.k"
static inline app_mm_Cycle    keel_tagged_app_mm_Cycle_void_tag  (const keel_tagged_app_mm_Cycle_void *t) { return (app_mm_Cycle)t->tag; }
#line 21 "keel/tagged.k"
static inline void keel_tagged_app_mm_Cycle_void_mark (keel_tagged_app_mm_Cycle_void *t, app_mm_Cycle e) { t->tag = e; }

#endif /* KEEL_KEEL_TAGGED_APP_MM_CYCLE_VOID_H */
