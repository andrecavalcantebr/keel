/* keel/keel_tagged_app_lib_other_Phase_void.h — generated from keel/tagged.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_TAGGED_APP_LIB_OTHER_PHASE_VOID_H
#define KEEL_KEEL_TAGGED_APP_LIB_OTHER_PHASE_VOID_H

#include "keel/keel_tagged_app_lib_other_Phase_void.type.h"
#include "app/lib/app_lib_other.type.h"

#line 18 "keel/tagged.k"
static inline app_lib_other_Phase    keel_tagged_app_lib_other_Phase_void_tag  (const keel_tagged_app_lib_other_Phase_void *t);
#line 21 "keel/tagged.k"
static inline void keel_tagged_app_lib_other_Phase_void_mark (keel_tagged_app_lib_other_Phase_void *t, app_lib_other_Phase e);

#line 18 "keel/tagged.k"
static inline app_lib_other_Phase    keel_tagged_app_lib_other_Phase_void_tag  (const keel_tagged_app_lib_other_Phase_void *t) { return (app_lib_other_Phase)t->tag; }
#line 21 "keel/tagged.k"
static inline void keel_tagged_app_lib_other_Phase_void_mark (keel_tagged_app_lib_other_Phase_void *t, app_lib_other_Phase e) { t->tag = e; }

#endif /* KEEL_KEEL_TAGGED_APP_LIB_OTHER_PHASE_VOID_H */
