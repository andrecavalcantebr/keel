/* keel/keel_slice_of_keel_array_i32.h — generated from keel/slice.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_SLICE_OF_KEEL_ARRAY_I32_H
#define KEEL_KEEL_SLICE_OF_KEEL_ARRAY_I32_H
#include "keel/keel_slice_of_keel_array_i32.type.h"
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_range.type.h"

#line 54 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of_keel_array_i32(i32 x[], size_t keel__N);
#line 55 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of1_keel_array_i32(i32 x[], size_t keel__N, keel_range r);
#line 56 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of2_keel_array_i32(i32 x[], size_t keel__N, size_t a, size_t b);
#include "keel/keel_array_i32.h"

#line 54 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of_keel_array_i32(i32 x[], size_t keel__N) { return keel_array_i32_as_slice2(x, keel__N, 0, keel_array_i32_length(x, keel__N)); }
#line 55 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of1_keel_array_i32(i32 x[], size_t keel__N, keel_range r) { return keel_array_i32_as_slice2(x, keel__N, r.first, r.limit); }
#line 56 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of2_keel_array_i32(i32 x[], size_t keel__N, size_t a, size_t b) { return keel_array_i32_as_slice2(x, keel__N, a, b); }
#endif /* KEEL_KEEL_SLICE_OF_KEEL_ARRAY_I32_H */
