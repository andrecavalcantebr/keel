/* keel/keel_tagged_pt_Mode_void.h — generated from keel/tagged.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_TAGGED_PT_MODE_VOID_H
#define KEEL_KEEL_TAGGED_PT_MODE_VOID_H

#include "keel/keel_tagged_pt_Mode_void.type.h"
#include "pt.type.h"

#line 18 "keel/tagged.k"
static inline pt_Mode    keel_tagged_pt_Mode_void_tag  (const keel_tagged_pt_Mode_void *t);
#line 21 "keel/tagged.k"
static inline void keel_tagged_pt_Mode_void_mark (keel_tagged_pt_Mode_void *t, pt_Mode e);

#line 18 "keel/tagged.k"
static inline pt_Mode    keel_tagged_pt_Mode_void_tag  (const keel_tagged_pt_Mode_void *t) { return (pt_Mode)t->tag; }
#line 21 "keel/tagged.k"
static inline void keel_tagged_pt_Mode_void_mark (keel_tagged_pt_Mode_void *t, pt_Mode e) { t->tag = e; }

#endif /* KEEL_KEEL_TAGGED_PT_MODE_VOID_H */
