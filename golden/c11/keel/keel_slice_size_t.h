/* keel/keel_slice_size_t.h — generated from keel/slice.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_SLICE_SIZE_T_H
#define KEEL_KEEL_SLICE_SIZE_T_H
#include "keel/keel_slice_size_t.type.h"
#include "keel/keel_slice.type.h"
#include "keel/keel_outcome_size_t.type.h"
#include "keel/keel_outcome_keel_slice_size_t.type.h"

typedef struct keel_arena keel_arena;

#line 25 "keel/slice.k"
static inline keel_slice_size_t keel_slice_size_t_from(size_t *p, size_t n);
#line 29 "keel/slice.k"
static inline size_t keel_slice_size_t_length(keel_slice_size_t s);
#line 30 "keel/slice.k"
static inline size_t    keel_slice_size_t_get (keel_slice_size_t s, size_t i);
#line 31 "keel/slice.k"
static inline void   keel_slice_size_t_set (keel_slice_size_t s, size_t i, size_t v);
#line 32 "keel/slice.k"
static inline size_t   *keel_slice_size_t_ptr (keel_slice_size_t s);
#line 33 "keel/slice.k"
static inline size_t   *keel_slice_size_t_ptr1(keel_slice_size_t s, size_t i);
#line 36 "keel/slice.k"
static inline keel_outcome_size_t keel_slice_size_t_at(keel_slice_size_t s, size_t i);
#line 43 "keel/slice.k"
static inline keel_slice_size_t keel_slice_size_t_of(keel_slice_size_t s, size_t a, size_t b);
#line 49 "keel/slice.k"
static inline keel_outcome_keel_slice_size_t keel_slice_size_t_clone(keel_arena *a, keel_slice_size_t s);
#line 58 "keel/slice.k"
static inline keel_slice_cursor keel_slice_size_t_begin(keel_slice_size_t s);
#line 59 "keel/slice.k"
static inline bool keel_slice_size_t_has_next(keel_slice_size_t s, keel_slice_cursor *c);
#line 60 "keel/slice.k"
static inline size_t *keel_slice_size_t_next(keel_slice_size_t s, keel_slice_cursor *c);
#line 64 "keel/slice.k"
static inline keel_slice_size_t keel_slice_size_t_partition(keel_slice_size_t s, size_t k, size_t w);
#include "keel/keel_arena.h"
#include "keel/keel_outcome_size_t.h"
#include "keel/keel_outcome_keel_slice_size_t.h"

#line 25 "keel/slice.k"
static inline keel_slice_size_t keel_slice_size_t_from(size_t *p, size_t n) {
    return (keel_slice_size_t){ p ? n : 0, p };
}
#line 29 "keel/slice.k"
static inline size_t keel_slice_size_t_length(keel_slice_size_t s) { return s.len; }
#line 30 "keel/slice.k"
static inline size_t    keel_slice_size_t_get (keel_slice_size_t s, size_t i) { return s.ptr[i]; }
#line 31 "keel/slice.k"
static inline void   keel_slice_size_t_set (keel_slice_size_t s, size_t i, size_t v) { s.ptr[i] = v; }
#line 32 "keel/slice.k"
static inline size_t   *keel_slice_size_t_ptr (keel_slice_size_t s) { return s.ptr; }
#line 33 "keel/slice.k"
static inline size_t   *keel_slice_size_t_ptr1(keel_slice_size_t s, size_t i) { return &s.ptr[i]; }
#line 36 "keel/slice.k"
static inline keel_outcome_size_t keel_slice_size_t_at(keel_slice_size_t s, size_t i) {
    keel_outcome_size_t r = {0};
    return i < s.len ? keel_outcome_size_t_win1(&r, s.ptr[i]) : keel_outcome_size_t_none(&r);
}
#line 43 "keel/slice.k"
static inline keel_slice_size_t keel_slice_size_t_of(keel_slice_size_t s, size_t a, size_t b) {
    if (b > s.len) b = s.len; if (a > b) a = b;
    return (keel_slice_size_t){ b - a, s.ptr + a };
}
#line 49 "keel/slice.k"
static inline keel_outcome_keel_slice_size_t keel_slice_size_t_clone(keel_arena *a, keel_slice_size_t s) {
    keel_outcome_keel_slice_size_t r = {0};
    size_t *data = (size_t *)keel_arena_alloc2(a, sizeof(size_t), _Alignof(size_t), s.len);
    if (!data) return keel_outcome_keel_slice_size_t_none(&r);
    if (s.len > 0) memcpy(data, s.ptr, s.len * sizeof(size_t));
    return keel_outcome_keel_slice_size_t_win1(&r, (keel_slice_size_t){ s.len, data });
}
#line 58 "keel/slice.k"
static inline keel_slice_cursor keel_slice_size_t_begin(keel_slice_size_t s) { (void)s; return (keel_slice_cursor){0}; }
#line 59 "keel/slice.k"
static inline bool   keel_slice_size_t_has_next(keel_slice_size_t s, keel_slice_cursor *c) { return c->i < s.len; }
#line 60 "keel/slice.k"
static inline size_t   *keel_slice_size_t_next(keel_slice_size_t s, keel_slice_cursor *c) { return &s.ptr[c->i++]; }
#line 64 "keel/slice.k"
static inline keel_slice_size_t keel_slice_size_t_partition(keel_slice_size_t s, size_t k, size_t w) {
    if (k == 0) return (keel_slice_size_t){0, s.ptr};
    size_t step = s.len / k + (s.len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= s.len) return (keel_slice_size_t){0, s.ptr};
    size_t hi = lo + step;
    if (hi > s.len) hi = s.len;
    return (keel_slice_size_t){ hi - lo, s.ptr + lo };
}
#endif /* KEEL_KEEL_SLICE_SIZE_T_H */
