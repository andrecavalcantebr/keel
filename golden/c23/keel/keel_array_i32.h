/* keel/keel_array_i32.h — generated from keel/array.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_ARRAY_I32_H
#define KEEL_KEEL_ARRAY_I32_H
#include "keel/keel_array_i32.type.h"
#include "keel/keel_outcome_i32.type.h"
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_range.type.h"

#line 21 "keel/array.k"
static inline i32    keel_array_i32_get(i32 v[], size_t keel__N, size_t i);
#line 22 "keel/array.k"
static inline void keel_array_i32_set(i32 v[], size_t keel__N, size_t i, i32 x);
#line 23 "keel/array.k"
static inline i32   *keel_array_i32_ptr(i32 v[], size_t keel__N);
#line 24 "keel/array.k"
static inline i32   *keel_array_i32_ptr1(i32 v[], size_t keel__N, size_t i);
#line 27 "keel/array.k"
static inline keel_outcome_i32 keel_array_i32_at(i32 v[], size_t keel__N, size_t i);
#line 37 "keel/array.k"
static inline keel_slice_i32 keel_array_i32_as_slice (i32 v[], size_t keel__N);
#line 39 "keel/array.k"
static inline keel_slice_i32 keel_array_i32_as_slice2(i32 v[], size_t keel__N, size_t a, size_t b);
#line 45 "keel/array.k"
static inline keel_slice_i32 keel_array_i32_as_slice1(i32 v[], size_t keel__N, keel_range r);
#line 49 "keel/array.k"
static inline size_t keel_array_i32_length(i32 v[], size_t keel__N);
#include "keel/keel_outcome_i32.h"

#line 21 "keel/array.k"
static inline i32    keel_array_i32_get(i32 v[], size_t keel__N, size_t i)      { return v[keel_index(i, keel__N)]; }
#line 22 "keel/array.k"
static inline void keel_array_i32_set(i32 v[], size_t keel__N, size_t i, i32 x) { v[keel_index(i, keel__N)] = x; }
#line 23 "keel/array.k"
static inline i32   *keel_array_i32_ptr(i32 v[], size_t keel__N)                { (void)keel__N; return v; }
#line 24 "keel/array.k"
static inline i32   *keel_array_i32_ptr1(i32 v[], size_t keel__N, size_t i)      { return &v[keel_index(i, keel__N)]; }
#line 27 "keel/array.k"
static inline keel_outcome_i32 keel_array_i32_at(i32 v[], size_t keel__N, size_t i) {
    keel_outcome_i32 r = {0};
    return i < keel__N ? keel_outcome_i32_win1(&r, v[keel_index(i, keel__N)]) : keel_outcome_i32_none(&r);
}
#line 37 "keel/array.k"
static inline keel_slice_i32 keel_array_i32_as_slice (i32 v[], size_t keel__N) { return (keel_slice_i32){ keel__N, v }; }
#line 39 "keel/array.k"
static inline keel_slice_i32 keel_array_i32_as_slice2(i32 v[], size_t keel__N, size_t a, size_t b) {
    KEEL_CHECK(a <= b && b <= keel__N, "range-index-out-of-bounds");
    if (b > keel__N) b = keel__N;
    if (a > b) a = b;
    return (keel_slice_i32){ b - a, v + a };
}
#line 45 "keel/array.k"
static inline keel_slice_i32 keel_array_i32_as_slice1(i32 v[], size_t keel__N, keel_range r) { return keel_array_i32_as_slice2(v, keel__N, r.first, r.limit); }
#line 49 "keel/array.k"
static inline size_t keel_array_i32_length(i32 v[], size_t keel__N) { (void)v; return keel__N; }
#endif /* KEEL_KEEL_ARRAY_I32_H */
