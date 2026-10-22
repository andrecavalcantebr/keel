/* keel/keel_outcome_f64.h — generated from keel/outcome.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_OUTCOME_F64_H
#define KEEL_KEEL_OUTCOME_F64_H
#include "keel/keel_outcome_f64.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_f64_failed(keel_outcome_f64 e);
static inline bool keel_outcome_f64_ok    (keel_outcome_f64 e);
static inline i32  keel_outcome_f64_code  (keel_outcome_f64 e);
static inline f64 keel_outcome_f64_value (keel_outcome_f64 e);
static inline void keel_outcome_f64_value1(keel_outcome_f64 *r, f64 v);
#line 29 "keel/outcome.k"
static inline keel_outcome_f64 keel_outcome_f64_win (keel_outcome_f64 *r);
static inline keel_outcome_f64 keel_outcome_f64_win1(keel_outcome_f64 *r, f64 v);
static inline keel_outcome_f64 keel_outcome_f64_fail(keel_outcome_f64 *r, i32 c);
static inline keel_outcome_f64 keel_outcome_f64_none(keel_outcome_f64 *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_f64_failed(keel_outcome_f64 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_f64_ok    (keel_outcome_f64 e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_f64_code  (keel_outcome_f64 e) { return e.code; }
static inline f64 keel_outcome_f64_value (keel_outcome_f64 e) { return e.value; }
static inline void keel_outcome_f64_value1(keel_outcome_f64 *r, f64 v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_f64 keel_outcome_f64_win (keel_outcome_f64 *r)        { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_f64 keel_outcome_f64_win1(keel_outcome_f64 *r, f64 v) { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_f64 keel_outcome_f64_fail(keel_outcome_f64 *r, i32 c) { r->code = c; return *r; }
static inline keel_outcome_f64 keel_outcome_f64_none(keel_outcome_f64 *r)        { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_F64_H */
