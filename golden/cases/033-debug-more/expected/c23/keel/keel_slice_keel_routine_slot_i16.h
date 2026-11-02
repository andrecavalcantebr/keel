/* keel/keel_slice_keel_routine_slot_i16.h — generated from keel/slice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_I16_H
#define KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_I16_H

#include "keel/keel_slice_keel_routine_slot_i16.type.h"
#include "keel/keel_slice.type.h"
#include "keel/keel_outcome_keel_routine_slot_i16.type.h"
#include "keel/keel_outcome_keel_slice_keel_routine_slot_i16.type.h"

typedef struct keel_arena keel_arena;
#line 26 "keel/slice.k"
static inline keel_slice_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_from(keel_routine_slot_i16 *p, size_t n);
#line 30 "keel/slice.k"
static inline size_t keel_slice_keel_routine_slot_i16_length(keel_slice_keel_routine_slot_i16 s);
static inline keel_routine_slot_i16      keel_slice_keel_routine_slot_i16_get(keel_slice_keel_routine_slot_i16 s, size_t i);
static inline void   keel_slice_keel_routine_slot_i16_set(keel_slice_keel_routine_slot_i16 s, size_t i, keel_routine_slot_i16 v);
static inline keel_routine_slot_i16     *keel_slice_keel_routine_slot_i16_ptr(keel_slice_keel_routine_slot_i16 s);
static inline keel_routine_slot_i16     *keel_slice_keel_routine_slot_i16_ptr1(keel_slice_keel_routine_slot_i16 s, size_t i);
#line 37 "keel/slice.k"
static inline keel_outcome_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_at(keel_slice_keel_routine_slot_i16 s, size_t i);
#line 45 "keel/slice.k"
static inline keel_slice_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_as_slice(keel_slice_keel_routine_slot_i16 s, size_t a, size_t b);
#line 59 "keel/slice.k"
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_clone(keel_arena *a, keel_slice_keel_routine_slot_i16 s);
#line 68 "keel/slice.k"
static inline keel_slice_cursor keel_slice_keel_routine_slot_i16_begin(keel_slice_keel_routine_slot_i16 s);
static inline bool   keel_slice_keel_routine_slot_i16_has_next(keel_slice_keel_routine_slot_i16 s, keel_slice_cursor *c);
static inline keel_routine_slot_i16     *keel_slice_keel_routine_slot_i16_next(keel_slice_keel_routine_slot_i16 s, keel_slice_cursor *c);
#line 74 "keel/slice.k"
static inline keel_slice_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_partition(keel_slice_keel_routine_slot_i16 s, size_t k, size_t w);

#include "keel/keel_arena.h"
#include "keel/keel_outcome_keel_routine_slot_i16.h"
#include "keel/keel_outcome_keel_slice_keel_routine_slot_i16.h"

#line 26 "keel/slice.k"
static inline keel_slice_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_from(keel_routine_slot_i16 *p, size_t n) {
    return (keel_slice_keel_routine_slot_i16){ p ? n : 0, p };
}
#line 30 "keel/slice.k"
static inline size_t keel_slice_keel_routine_slot_i16_length(keel_slice_keel_routine_slot_i16 s) { return s.len; }
static inline keel_routine_slot_i16      keel_slice_keel_routine_slot_i16_get(keel_slice_keel_routine_slot_i16 s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return s.ptr[i]; }
static inline void   keel_slice_keel_routine_slot_i16_set(keel_slice_keel_routine_slot_i16 s, size_t i, keel_routine_slot_i16 v) { KEEL_CHECK(i < s.len, "set-out-of-length"); s.ptr[i] = v; }
static inline keel_routine_slot_i16     *keel_slice_keel_routine_slot_i16_ptr(keel_slice_keel_routine_slot_i16 s) { return s.ptr; }
static inline keel_routine_slot_i16     *keel_slice_keel_routine_slot_i16_ptr1(keel_slice_keel_routine_slot_i16 s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return &s.ptr[i]; }
#line 37 "keel/slice.k"
static inline keel_outcome_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_at(keel_slice_keel_routine_slot_i16 s, size_t i) {
    keel_outcome_keel_routine_slot_i16 r = {0};
    return i < s.len ? keel_outcome_keel_routine_slot_i16_win1(&r, s.ptr[i]) : keel_outcome_keel_routine_slot_i16_none(&r);
}
#line 45 "keel/slice.k"
static inline keel_slice_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_as_slice(keel_slice_keel_routine_slot_i16 s, size_t a, size_t b) {
#line 45 "keel/slice.k"
    KEEL_CHECK(a <= b && b <= keel_slice_keel_routine_slot_i16_length(s), "range-index-out-of-bounds");
    if (b > s.len) b = s.len;
    if (a > b) a = b;
    return (keel_slice_keel_routine_slot_i16){ b - a, s.ptr + a };
}
#line 59 "keel/slice.k"
static inline keel_outcome_keel_slice_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_clone(keel_arena *a, keel_slice_keel_routine_slot_i16 s) {
    keel_outcome_keel_slice_keel_routine_slot_i16 r = {0};
    keel_routine_slot_i16 *data = (keel_routine_slot_i16 *)keel_arena_alloc2(a, sizeof(keel_routine_slot_i16), alignof(keel_routine_slot_i16), s.len);
    if (!data) return keel_outcome_keel_slice_keel_routine_slot_i16_none(&r);
    if (s.len > 0) memcpy((void *)data, s.ptr, s.len * sizeof(keel_routine_slot_i16));
    return keel_outcome_keel_slice_keel_routine_slot_i16_win1(&r, (keel_slice_keel_routine_slot_i16){ s.len, data });
}
#line 68 "keel/slice.k"
static inline keel_slice_cursor keel_slice_keel_routine_slot_i16_begin(keel_slice_keel_routine_slot_i16 s) { (void)s; return (keel_slice_cursor){0}; }
static inline bool   keel_slice_keel_routine_slot_i16_has_next(keel_slice_keel_routine_slot_i16 s, keel_slice_cursor *c) { return c->i < s.len; }
static inline keel_routine_slot_i16     *keel_slice_keel_routine_slot_i16_next(keel_slice_keel_routine_slot_i16 s, keel_slice_cursor *c) { return &s.ptr[c->i++]; }
#line 74 "keel/slice.k"
static inline keel_slice_keel_routine_slot_i16 keel_slice_keel_routine_slot_i16_partition(keel_slice_keel_routine_slot_i16 s, size_t k, size_t w) {
    if (k == 0) return (keel_slice_keel_routine_slot_i16){0, s.ptr};
    size_t step = s.len / k + (s.len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= s.len) return (keel_slice_keel_routine_slot_i16){0, s.ptr};
    size_t hi = lo + step;
    if (hi > s.len) hi = s.len;
    return (keel_slice_keel_routine_slot_i16){ hi - lo, s.ptr + lo };
}

#endif /* KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_I16_H */
