/* keel/keel_outcome_keel_slice_keel_routine_slot_i16.h — generated from keel/outcome.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_SLICE_KEEL_ROUTINE_SLOT_I16_H
#define KEEL_KEEL_OUTCOME_KEEL_SLICE_KEEL_ROUTINE_SLOT_I16_H

#include "keel/keel_outcome_keel_slice_keel_routine_slot_i16.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_slice_keel_routine_slot_i16_failed(keel_outcome_keel_slice_keel_routine_slot_i16 e);
static inline bool keel_outcome_keel_slice_keel_routine_slot_i16_ok    (keel_outcome_keel_slice_keel_routine_slot_i16 e);
static inline i32  keel_outcome_keel_slice_keel_routine_slot_i16_code  (keel_outcome_keel_slice_keel_routine_slot_i16 e);
static inline keel_slice_keel_routine_slot_i16    keel_outcome_keel_slice_keel_routine_slot_i16_value (keel_outcome_keel_slice_keel_routine_slot_i16 e);
static inline void keel_outcome_keel_slice_keel_routine_slot_i16_value1 (keel_outcome_keel_slice_keel_routine_slot_i16 *r, keel_slice_keel_routine_slot_i16 v);
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_outcome_keel_slice_keel_routine_slot_i16_win (keel_outcome_keel_slice_keel_routine_slot_i16 *r);
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_outcome_keel_slice_keel_routine_slot_i16_win1 (keel_outcome_keel_slice_keel_routine_slot_i16 *r, keel_slice_keel_routine_slot_i16 v);
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_outcome_keel_slice_keel_routine_slot_i16_fail(keel_outcome_keel_slice_keel_routine_slot_i16 *r, i32 c);
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_outcome_keel_slice_keel_routine_slot_i16_none(keel_outcome_keel_slice_keel_routine_slot_i16 *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_slice_keel_routine_slot_i16_failed(keel_outcome_keel_slice_keel_routine_slot_i16 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_keel_slice_keel_routine_slot_i16_ok    (keel_outcome_keel_slice_keel_routine_slot_i16 e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_keel_slice_keel_routine_slot_i16_code  (keel_outcome_keel_slice_keel_routine_slot_i16 e) { return e.code; }
static inline keel_slice_keel_routine_slot_i16    keel_outcome_keel_slice_keel_routine_slot_i16_value (keel_outcome_keel_slice_keel_routine_slot_i16 e) { return e.value; }
static inline void keel_outcome_keel_slice_keel_routine_slot_i16_value1 (keel_outcome_keel_slice_keel_routine_slot_i16 *r, keel_slice_keel_routine_slot_i16 v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_outcome_keel_slice_keel_routine_slot_i16_win (keel_outcome_keel_slice_keel_routine_slot_i16 *r)         { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_outcome_keel_slice_keel_routine_slot_i16_win1 (keel_outcome_keel_slice_keel_routine_slot_i16 *r, keel_slice_keel_routine_slot_i16 v)    { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_outcome_keel_slice_keel_routine_slot_i16_fail(keel_outcome_keel_slice_keel_routine_slot_i16 *r, i32 c)  { r->code = c; return *r; }
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_outcome_keel_slice_keel_routine_slot_i16_none(keel_outcome_keel_slice_keel_routine_slot_i16 *r)         { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_KEEL_SLICE_KEEL_ROUTINE_SLOT_I16_H */
