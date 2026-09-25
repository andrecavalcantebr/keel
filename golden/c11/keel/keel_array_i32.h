/* keel/keel_array_i32.h — generated from keel/array.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_ARRAY_I32_H
#define KEEL_KEEL_ARRAY_I32_H
#include "keel/keel_array_i32.type.h"
#include "keel/keel_outcome_i32.type.h"

#line 19 "keel/array.k"
static inline i32    keel_array_i32_get(i32 v[], size_t keel__N, size_t i);
#line 20 "keel/array.k"
static inline void keel_array_i32_set(i32 v[], size_t keel__N, size_t i, i32 x);
#line 21 "keel/array.k"
static inline i32   *keel_array_i32_ptr(i32 v[], size_t keel__N);
#line 22 "keel/array.k"
static inline i32   *keel_array_i32_ptr1(i32 v[], size_t keel__N, size_t i);
#line 25 "keel/array.k"
static inline keel_outcome_i32 keel_array_i32_at(i32 v[], size_t keel__N, size_t i);
#include "keel/keel_outcome_i32.h"

#line 19 "keel/array.k"
static inline i32    keel_array_i32_get(i32 v[], size_t keel__N, size_t i)      { return v[keel_index(i, keel__N)]; }
#line 20 "keel/array.k"
static inline void keel_array_i32_set(i32 v[], size_t keel__N, size_t i, i32 x) { v[keel_index(i, keel__N)] = x; }
#line 21 "keel/array.k"
static inline i32   *keel_array_i32_ptr(i32 v[], size_t keel__N)                { (void)keel__N; return v; }
#line 22 "keel/array.k"
static inline i32   *keel_array_i32_ptr1(i32 v[], size_t keel__N, size_t i)      { return &v[keel_index(i, keel__N)]; }
#line 25 "keel/array.k"
static inline keel_outcome_i32 keel_array_i32_at(i32 v[], size_t keel__N, size_t i) {
    keel_outcome_i32 r = {0};
    return i < keel__N ? keel_outcome_i32_win1(&r, v[keel_index(i, keel__N)]) : keel_outcome_i32_none(&r);
}
#endif /* KEEL_KEEL_ARRAY_I32_H */
