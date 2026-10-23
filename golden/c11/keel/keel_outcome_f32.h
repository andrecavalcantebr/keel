/* keel/keel_outcome_f32.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_F32_H
#define KEEL_KEEL_OUTCOME_F32_H

#include "keel/keel_outcome_f32.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_f32_failed(keel_outcome_f32 e);
static inline bool keel_outcome_f32_ok    (keel_outcome_f32 e);
static inline i32  keel_outcome_f32_code  (keel_outcome_f32 e);
static inline f32 keel_outcome_f32_value (keel_outcome_f32 e);
static inline void keel_outcome_f32_value1(keel_outcome_f32 *r, f32 v);
#line 29 "keel/outcome.k"
static inline keel_outcome_f32 keel_outcome_f32_win (keel_outcome_f32 *r);
static inline keel_outcome_f32 keel_outcome_f32_win1(keel_outcome_f32 *r, f32 v);
static inline keel_outcome_f32 keel_outcome_f32_fail(keel_outcome_f32 *r, i32 c);
static inline keel_outcome_f32 keel_outcome_f32_none(keel_outcome_f32 *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_f32_failed(keel_outcome_f32 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_f32_ok    (keel_outcome_f32 e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_f32_code  (keel_outcome_f32 e) { return e.code; }
static inline f32 keel_outcome_f32_value (keel_outcome_f32 e) { return e.value; }
static inline void keel_outcome_f32_value1(keel_outcome_f32 *r, f32 v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_f32 keel_outcome_f32_win (keel_outcome_f32 *r)        { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_f32 keel_outcome_f32_win1(keel_outcome_f32 *r, f32 v) { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_f32 keel_outcome_f32_fail(keel_outcome_f32 *r, i32 c) { r->code = c; return *r; }
static inline keel_outcome_f32 keel_outcome_f32_none(keel_outcome_f32 *r)        { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_F32_H */
