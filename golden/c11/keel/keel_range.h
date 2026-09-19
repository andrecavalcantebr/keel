/* keel/keel_range.h — generated from keel/range.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_RANGE_H
#define KEEL_KEEL_RANGE_H
#include "keel/keel_range.type.h"

static inline keel_range keel_range_of(size_t a, size_t b);
static inline size_t keel_range_first (keel_range r);
static inline size_t keel_range_limit (keel_range r);
static inline size_t keel_range_length(keel_range r);
static inline size_t keel_range_get   (keel_range r, size_t i);

static inline keel_range keel_range_of(size_t a, size_t b) { return (keel_range){ a, b < a ? a : b }; }
static inline size_t keel_range_first (keel_range r) { return r.first; }
static inline size_t keel_range_limit (keel_range r) { return r.limit; }
static inline size_t keel_range_length(keel_range r) { return r.limit - r.first; }
static inline size_t keel_range_get   (keel_range r, size_t i) { return r.first + i; }
#endif /* KEEL_KEEL_RANGE_H */
