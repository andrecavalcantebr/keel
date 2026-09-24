/* keel/keel_slice_i32.h — generated from keel/slice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_SLICE_I32_H
#define KEEL_KEEL_SLICE_I32_H
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_slice.type.h"
#include "keel/keel_outcome_i32.type.h"
#include "keel/keel_outcome_keel_slice_i32.type.h"

typedef struct keel_arena keel_arena;

#line 25 "keel/slice.k"
static inline keel_slice_i32 keel_slice_i32_from(i32 *p, size_t n);
#line 29 "keel/slice.k"
static inline size_t keel_slice_i32_length(keel_slice_i32 s);
#line 30 "keel/slice.k"
static inline i32    keel_slice_i32_get (keel_slice_i32 s, size_t i);
#line 31 "keel/slice.k"
static inline void   keel_slice_i32_set (keel_slice_i32 s, size_t i, i32 v);
#line 32 "keel/slice.k"
static inline i32   *keel_slice_i32_ptr (keel_slice_i32 s);
#line 33 "keel/slice.k"
static inline i32   *keel_slice_i32_ptr1(keel_slice_i32 s, size_t i);
#line 36 "keel/slice.k"
static inline keel_outcome_i32 keel_slice_i32_at(keel_slice_i32 s, size_t i);
#line 43 "keel/slice.k"
static inline keel_slice_i32 keel_slice_i32_of(keel_slice_i32 s, size_t a, size_t b);
#line 49 "keel/slice.k"
static inline keel_outcome_keel_slice_i32 keel_slice_i32_clone(keel_arena *a, keel_slice_i32 s);
#line 58 "keel/slice.k"
static inline keel_slice_cursor keel_slice_i32_begin(keel_slice_i32 s);
#line 59 "keel/slice.k"
static inline bool keel_slice_i32_has_next(keel_slice_i32 s, keel_slice_cursor *c);
#line 60 "keel/slice.k"
static inline i32 *keel_slice_i32_next(keel_slice_i32 s, keel_slice_cursor *c);
#line 64 "keel/slice.k"
static inline keel_slice_i32 keel_slice_i32_partition(keel_slice_i32 s, size_t k, size_t w);
#include "keel/keel_arena.h"
#include "keel/keel_outcome_i32.h"
#include "keel/keel_outcome_keel_slice_i32.h"

#line 25 "keel/slice.k"
static inline keel_slice_i32 keel_slice_i32_from(i32 *p, size_t n) {
    return (keel_slice_i32){ p ? n : 0, p };
}
#line 29 "keel/slice.k"
static inline size_t keel_slice_i32_length(keel_slice_i32 s) { return s.len; }
#line 30 "keel/slice.k"
static inline i32    keel_slice_i32_get (keel_slice_i32 s, size_t i) { return s.ptr[i]; }
#line 31 "keel/slice.k"
static inline void   keel_slice_i32_set (keel_slice_i32 s, size_t i, i32 v) { s.ptr[i] = v; }
#line 32 "keel/slice.k"
static inline i32   *keel_slice_i32_ptr (keel_slice_i32 s) { return s.ptr; }
#line 33 "keel/slice.k"
static inline i32   *keel_slice_i32_ptr1(keel_slice_i32 s, size_t i) { return &s.ptr[i]; }
#line 36 "keel/slice.k"
static inline keel_outcome_i32 keel_slice_i32_at(keel_slice_i32 s, size_t i) {
    keel_outcome_i32 r = {0};
    return i < s.len ? keel_outcome_i32_win1(&r, s.ptr[i]) : keel_outcome_i32_none(&r);
}
#line 43 "keel/slice.k"
static inline keel_slice_i32 keel_slice_i32_of(keel_slice_i32 s, size_t a, size_t b) {
    if (a > b || b > s.len) return (keel_slice_i32){0, s.ptr};
    return (keel_slice_i32){ b - a, s.ptr + a };
}
#line 49 "keel/slice.k"
static inline keel_outcome_keel_slice_i32 keel_slice_i32_clone(keel_arena *a, keel_slice_i32 s) {
    keel_outcome_keel_slice_i32 r = {0};
    i32 *data = (i32 *)keel_arena_alloc2(a, sizeof(i32), alignof(i32), s.len);
    if (!data) return keel_outcome_keel_slice_i32_none(&r);
    if (s.len > 0) memcpy(data, s.ptr, s.len * sizeof(i32));
    return keel_outcome_keel_slice_i32_win1(&r, (keel_slice_i32){ s.len, data });
}
#line 58 "keel/slice.k"
static inline keel_slice_cursor keel_slice_i32_begin(keel_slice_i32 s) { (void)s; return (keel_slice_cursor){0}; }
#line 59 "keel/slice.k"
static inline bool   keel_slice_i32_has_next(keel_slice_i32 s, keel_slice_cursor *c) { return c->i < s.len; }
#line 60 "keel/slice.k"
static inline i32   *keel_slice_i32_next(keel_slice_i32 s, keel_slice_cursor *c) { return &s.ptr[c->i++]; }
#line 64 "keel/slice.k"
static inline keel_slice_i32 keel_slice_i32_partition(keel_slice_i32 s, size_t k, size_t w) {
    if (k == 0) return (keel_slice_i32){0, s.ptr};
    size_t step = s.len / k + (s.len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= s.len) return (keel_slice_i32){0, s.ptr};
    size_t hi = lo + step;
    if (hi > s.len) hi = s.len;
    return (keel_slice_i32){ hi - lo, s.ptr + lo };
}
#endif /* KEEL_KEEL_SLICE_I32_H */
