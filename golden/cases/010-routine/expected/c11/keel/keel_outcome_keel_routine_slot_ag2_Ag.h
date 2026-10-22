/* keel/keel_outcome_keel_routine_slot_ag2_Ag.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_ROUTINE_SLOT_AG2_AG_H
#define KEEL_KEEL_OUTCOME_KEEL_ROUTINE_SLOT_AG2_AG_H
#include "keel/keel_outcome_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_routine_slot_ag2_Ag_failed(keel_outcome_keel_routine_slot_ag2_Ag e);
static inline bool keel_outcome_keel_routine_slot_ag2_Ag_ok    (keel_outcome_keel_routine_slot_ag2_Ag e);
static inline i32  keel_outcome_keel_routine_slot_ag2_Ag_code  (keel_outcome_keel_routine_slot_ag2_Ag e);
static inline keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_value (keel_outcome_keel_routine_slot_ag2_Ag e);
static inline void keel_outcome_keel_routine_slot_ag2_Ag_value1(keel_outcome_keel_routine_slot_ag2_Ag *r, keel_routine_slot_ag2_Ag v);
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_win (keel_outcome_keel_routine_slot_ag2_Ag *r);
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_win1(keel_outcome_keel_routine_slot_ag2_Ag *r, keel_routine_slot_ag2_Ag v);
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_fail(keel_outcome_keel_routine_slot_ag2_Ag *r, i32 c);
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_none(keel_outcome_keel_routine_slot_ag2_Ag *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_routine_slot_ag2_Ag_failed(keel_outcome_keel_routine_slot_ag2_Ag e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_keel_routine_slot_ag2_Ag_ok    (keel_outcome_keel_routine_slot_ag2_Ag e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_keel_routine_slot_ag2_Ag_code  (keel_outcome_keel_routine_slot_ag2_Ag e) { return e.code; }
static inline keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_value (keel_outcome_keel_routine_slot_ag2_Ag e) { return e.value; }
static inline void keel_outcome_keel_routine_slot_ag2_Ag_value1(keel_outcome_keel_routine_slot_ag2_Ag *r, keel_routine_slot_ag2_Ag v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_win (keel_outcome_keel_routine_slot_ag2_Ag *r)        { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_win1(keel_outcome_keel_routine_slot_ag2_Ag *r, keel_routine_slot_ag2_Ag v) { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_fail(keel_outcome_keel_routine_slot_ag2_Ag *r, i32 c) { r->code = c; return *r; }
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_outcome_keel_routine_slot_ag2_Ag_none(keel_outcome_keel_routine_slot_ag2_Ag *r)        { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_KEEL_ROUTINE_SLOT_AG2_AG_H */
