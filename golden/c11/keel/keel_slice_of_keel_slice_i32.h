/* keel/keel_slice_of_keel_slice_i32.h — generated from keel/slice.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_SLICE_OF_KEEL_SLICE_I32_H
#define KEEL_KEEL_SLICE_OF_KEEL_SLICE_I32_H
#include "keel/keel_slice_of_keel_slice_i32.type.h"
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_range.type.h"

#line 54 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of_keel_slice_i32(keel_slice_i32 x);
#line 55 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of1_keel_slice_i32(keel_slice_i32 x, keel_range r);
#line 56 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of2_keel_slice_i32(keel_slice_i32 x, size_t a, size_t b);
#include "keel/keel_slice_i32.h"

#line 54 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of_keel_slice_i32(keel_slice_i32 x) { return keel_slice_i32_as_slice(x, 0, keel_slice_i32_length(x)); }
#line 55 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of1_keel_slice_i32(keel_slice_i32 x, keel_range r) { return keel_slice_i32_as_slice(x, r.first, r.limit); }
#line 56 "keel/slice.k"
static inline keel_slice_i32 keel_slice_of2_keel_slice_i32(keel_slice_i32 x, size_t a, size_t b) { return keel_slice_i32_as_slice(x, a, b); }
#endif /* KEEL_KEEL_SLICE_OF_KEEL_SLICE_I32_H */
