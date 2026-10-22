/* keel/keel_outcome_u32.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_U32_H
#define KEEL_KEEL_OUTCOME_U32_H
#include "keel/keel_outcome_u32.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_u32_failed(keel_outcome_u32 e);
static inline bool keel_outcome_u32_ok    (keel_outcome_u32 e);
static inline i32  keel_outcome_u32_code  (keel_outcome_u32 e);
static inline u32 keel_outcome_u32_value (keel_outcome_u32 e);
static inline void keel_outcome_u32_value1(keel_outcome_u32 *r, u32 v);
#line 29 "keel/outcome.k"
static inline keel_outcome_u32 keel_outcome_u32_win (keel_outcome_u32 *r);
static inline keel_outcome_u32 keel_outcome_u32_win1(keel_outcome_u32 *r, u32 v);
static inline keel_outcome_u32 keel_outcome_u32_fail(keel_outcome_u32 *r, i32 c);
static inline keel_outcome_u32 keel_outcome_u32_none(keel_outcome_u32 *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_u32_failed(keel_outcome_u32 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_u32_ok    (keel_outcome_u32 e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_u32_code  (keel_outcome_u32 e) { return e.code; }
static inline u32 keel_outcome_u32_value (keel_outcome_u32 e) { return e.value; }
static inline void keel_outcome_u32_value1(keel_outcome_u32 *r, u32 v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_u32 keel_outcome_u32_win (keel_outcome_u32 *r)        { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_u32 keel_outcome_u32_win1(keel_outcome_u32 *r, u32 v) { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_u32 keel_outcome_u32_fail(keel_outcome_u32 *r, i32 c) { r->code = c; return *r; }
static inline keel_outcome_u32 keel_outcome_u32_none(keel_outcome_u32 *r)        { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_U32_H */
