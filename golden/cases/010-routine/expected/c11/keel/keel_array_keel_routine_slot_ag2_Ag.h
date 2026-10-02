/* keel/keel_array_keel_routine_slot_ag2_Ag.h — generated from keel/array.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_ARRAY_KEEL_ROUTINE_SLOT_AG2_AG_H
#define KEEL_KEEL_ARRAY_KEEL_ROUTINE_SLOT_AG2_AG_H
#include "keel/keel_array_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_outcome_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_range.type.h"

#line 21 "keel/array.k"
static inline keel_routine_slot_ag2_Ag    keel_array_keel_routine_slot_ag2_Ag_get(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t i);
#line 22 "keel/array.k"
static inline void keel_array_keel_routine_slot_ag2_Ag_set(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t i, keel_routine_slot_ag2_Ag x);
#line 23 "keel/array.k"
static inline keel_routine_slot_ag2_Ag   *keel_array_keel_routine_slot_ag2_Ag_ptr(keel_routine_slot_ag2_Ag v[], size_t keel__N);
#line 24 "keel/array.k"
static inline keel_routine_slot_ag2_Ag   *keel_array_keel_routine_slot_ag2_Ag_ptr1(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t i);
#line 27 "keel/array.k"
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_array_keel_routine_slot_ag2_Ag_at(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t i);
#line 37 "keel/array.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_array_keel_routine_slot_ag2_Ag_as_slice (keel_routine_slot_ag2_Ag v[], size_t keel__N);
#line 39 "keel/array.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_array_keel_routine_slot_ag2_Ag_as_slice2(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t a, size_t b);
#line 45 "keel/array.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_array_keel_routine_slot_ag2_Ag_as_slice1(keel_routine_slot_ag2_Ag v[], size_t keel__N, keel_range r);
#line 49 "keel/array.k"
static inline size_t keel_array_keel_routine_slot_ag2_Ag_length(keel_routine_slot_ag2_Ag v[], size_t keel__N);
#include "keel/keel_outcome_keel_routine_slot_ag2_Ag.h"

#line 21 "keel/array.k"
static inline keel_routine_slot_ag2_Ag    keel_array_keel_routine_slot_ag2_Ag_get(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t i)      { return v[keel_index(i, keel__N)]; }
#line 22 "keel/array.k"
static inline void keel_array_keel_routine_slot_ag2_Ag_set(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t i, keel_routine_slot_ag2_Ag x) { v[keel_index(i, keel__N)] = x; }
#line 23 "keel/array.k"
static inline keel_routine_slot_ag2_Ag   *keel_array_keel_routine_slot_ag2_Ag_ptr(keel_routine_slot_ag2_Ag v[], size_t keel__N)                { (void)keel__N; return v; }
#line 24 "keel/array.k"
static inline keel_routine_slot_ag2_Ag   *keel_array_keel_routine_slot_ag2_Ag_ptr1(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t i)      { return &v[keel_index(i, keel__N)]; }
#line 27 "keel/array.k"
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_array_keel_routine_slot_ag2_Ag_at(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t i) {
    keel_outcome_keel_routine_slot_ag2_Ag r = {0};
    return i < keel__N ? keel_outcome_keel_routine_slot_ag2_Ag_win1(&r, v[keel_index(i, keel__N)]) : keel_outcome_keel_routine_slot_ag2_Ag_none(&r);
}
#line 37 "keel/array.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_array_keel_routine_slot_ag2_Ag_as_slice (keel_routine_slot_ag2_Ag v[], size_t keel__N) { return (keel_slice_keel_routine_slot_ag2_Ag){ keel__N, v }; }
#line 39 "keel/array.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_array_keel_routine_slot_ag2_Ag_as_slice2(keel_routine_slot_ag2_Ag v[], size_t keel__N, size_t a, size_t b) {
    KEEL_CHECK(a <= b && b <= keel__N, "range-index-out-of-bounds");
    if (b > keel__N) b = keel__N;
    if (a > b) a = b;
    return (keel_slice_keel_routine_slot_ag2_Ag){ b - a, v + a };
}
#line 45 "keel/array.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_array_keel_routine_slot_ag2_Ag_as_slice1(keel_routine_slot_ag2_Ag v[], size_t keel__N, keel_range r) { return keel_array_keel_routine_slot_ag2_Ag_as_slice2(v, keel__N, r.first, r.limit); }
#line 49 "keel/array.k"
static inline size_t keel_array_keel_routine_slot_ag2_Ag_length(keel_routine_slot_ag2_Ag v[], size_t keel__N) { (void)v; return keel__N; }
#endif /* KEEL_KEEL_ARRAY_KEEL_ROUTINE_SLOT_AG2_AG_H */
