/* keel/keel_outcome_keel_slice_keel_routine_slot_ag2_Ag.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#define KEEL_KEEL_OUTCOME_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#include "keel/keel_outcome_keel_slice_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_failed(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag e);
#line 22 "keel/outcome.k"
static inline bool keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_ok    (keel_outcome_keel_slice_keel_routine_slot_ag2_Ag e);
#line 23 "keel/outcome.k"
static inline i32  keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_code  (keel_outcome_keel_slice_keel_routine_slot_ag2_Ag e);
#line 24 "keel/outcome.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_value (keel_outcome_keel_slice_keel_routine_slot_ag2_Ag e);
#line 25 "keel/outcome.k"
static inline void keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_value1(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r, keel_slice_keel_routine_slot_ag2_Ag v);
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_win (keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r);
#line 30 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_win1(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r, keel_slice_keel_routine_slot_ag2_Ag v);
#line 31 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_fail(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r, i32 c);
#line 32 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_none(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_failed(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag e) { return e.code != keel_outcome_OK; }
#line 22 "keel/outcome.k"
static inline bool keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_ok    (keel_outcome_keel_slice_keel_routine_slot_ag2_Ag e) { return e.code == keel_outcome_OK; }
#line 23 "keel/outcome.k"
static inline i32  keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_code  (keel_outcome_keel_slice_keel_routine_slot_ag2_Ag e) { return e.code; }
#line 24 "keel/outcome.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_value (keel_outcome_keel_slice_keel_routine_slot_ag2_Ag e) { return e.value; }
#line 25 "keel/outcome.k"
static inline void keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_value1(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r, keel_slice_keel_routine_slot_ag2_Ag v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_win (keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r)        { r->code = keel_outcome_OK; return *r; }
#line 30 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_win1(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r, keel_slice_keel_routine_slot_ag2_Ag v) { r->code = keel_outcome_OK; r->value = v; return *r; }
#line 31 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_fail(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r, i32 c) { r->code = c; return *r; }
#line 32 "keel/outcome.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_none(keel_outcome_keel_slice_keel_routine_slot_ag2_Ag *r)        { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H */
