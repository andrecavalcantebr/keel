/* keel/keel_slice_of_keel_array_keel_routine_slot_ag2_Ag.h — generated from keel/slice.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_SLICE_OF_KEEL_ARRAY_KEEL_ROUTINE_SLOT_AG2_AG_H
#define KEEL_KEEL_SLICE_OF_KEEL_ARRAY_KEEL_ROUTINE_SLOT_AG2_AG_H
#include "keel/keel_slice_of_keel_array_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_range.type.h"

#line 54 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_of_keel_array_keel_routine_slot_ag2_Ag(keel_routine_slot_ag2_Ag x[], size_t keel__N);
#line 55 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_of1_keel_array_keel_routine_slot_ag2_Ag(keel_routine_slot_ag2_Ag x[], size_t keel__N, keel_range r);
#line 56 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_of2_keel_array_keel_routine_slot_ag2_Ag(keel_routine_slot_ag2_Ag x[], size_t keel__N, size_t a, size_t b);
#include "keel/keel_array_keel_routine_slot_ag2_Ag.h"

#line 54 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_of_keel_array_keel_routine_slot_ag2_Ag(keel_routine_slot_ag2_Ag x[], size_t keel__N) { return keel_array_keel_routine_slot_ag2_Ag_as_slice2(x, keel__N, 0, keel_array_keel_routine_slot_ag2_Ag_length(x, keel__N)); }
#line 55 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_of1_keel_array_keel_routine_slot_ag2_Ag(keel_routine_slot_ag2_Ag x[], size_t keel__N, keel_range r) { return keel_array_keel_routine_slot_ag2_Ag_as_slice2(x, keel__N, r.first, r.limit); }
#line 56 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_of2_keel_array_keel_routine_slot_ag2_Ag(keel_routine_slot_ag2_Ag x[], size_t keel__N, size_t a, size_t b) { return keel_array_keel_routine_slot_ag2_Ag_as_slice2(x, keel__N, a, b); }
#endif /* KEEL_KEEL_SLICE_OF_KEEL_ARRAY_KEEL_ROUTINE_SLOT_AG2_AG_H */
