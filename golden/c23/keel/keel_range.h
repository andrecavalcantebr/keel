/* keel/keel_range.h — generated from keel/range.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_RANGE_H
#define KEEL_KEEL_RANGE_H
#include "keel/keel_range.type.h"

#line 16 "keel/range.k"
static inline keel_range keel_range_of(size_t a, size_t b);
#line 18 "keel/range.k"
static inline size_t keel_range_first (keel_range r);
#line 19 "keel/range.k"
static inline size_t keel_range_limit (keel_range r);
#line 20 "keel/range.k"
static inline size_t keel_range_length(keel_range r);
#line 21 "keel/range.k"
static inline size_t keel_range_get   (keel_range r, size_t i);
#line 28 "keel/range.k"
static inline keel_range keel_range_partition(keel_range r, size_t k, size_t w);

#line 16 "keel/range.k"
static inline keel_range keel_range_of(size_t a, size_t b) { return (keel_range){ a, b < a ? a : b }; }
#line 18 "keel/range.k"
static inline size_t keel_range_first (keel_range r) { return r.first; }
#line 19 "keel/range.k"
static inline size_t keel_range_limit (keel_range r) { return r.limit; }
#line 20 "keel/range.k"
static inline size_t keel_range_length(keel_range r) { return r.limit - r.first; }
#line 21 "keel/range.k"
static inline size_t keel_range_get   (keel_range r, size_t i) { return r.first + i; }
#line 28 "keel/range.k"
static inline keel_range keel_range_partition(keel_range r, size_t k, size_t w) {
    if (k == 0) return (keel_range){ r.first, r.first };
    size_t n = r.limit - r.first;
    size_t step = n / k + (n % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= n) return (keel_range){ r.limit, r.limit };
    size_t hi = lo + step;
    if (hi > n) hi = n;
    return (keel_range){ r.first + lo, r.first + hi };
}
#endif /* KEEL_KEEL_RANGE_H */
