/* keel/keel_outcome_void.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_VOID_H
#define KEEL_KEEL_OUTCOME_VOID_H
#include "keel/keel_outcome_void.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_void_failed(keel_outcome_void e);
#line 22 "keel/outcome.k"
static inline bool keel_outcome_void_ok    (keel_outcome_void e);
#line 23 "keel/outcome.k"
static inline i32  keel_outcome_void_code  (keel_outcome_void e);
#line 29 "keel/outcome.k"
static inline keel_outcome_void keel_outcome_void_win (keel_outcome_void *r);
#line 31 "keel/outcome.k"
static inline keel_outcome_void keel_outcome_void_fail(keel_outcome_void *r, i32 c);
#line 32 "keel/outcome.k"
static inline keel_outcome_void keel_outcome_void_none(keel_outcome_void *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_void_failed(keel_outcome_void e) { return e.code != keel_outcome_OK; }
#line 22 "keel/outcome.k"
static inline bool keel_outcome_void_ok    (keel_outcome_void e) { return e.code == keel_outcome_OK; }
#line 23 "keel/outcome.k"
static inline i32  keel_outcome_void_code  (keel_outcome_void e) { return e.code; }
#line 29 "keel/outcome.k"
static inline keel_outcome_void keel_outcome_void_win (keel_outcome_void *r)        { r->code = keel_outcome_OK; return *r; }
#line 31 "keel/outcome.k"
static inline keel_outcome_void keel_outcome_void_fail(keel_outcome_void *r, i32 c) { r->code = c; return *r; }
#line 32 "keel/outcome.k"
static inline keel_outcome_void keel_outcome_void_none(keel_outcome_void *r)        { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_VOID_H */
