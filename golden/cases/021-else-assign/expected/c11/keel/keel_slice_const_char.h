/* keel/keel_slice_const_char.h — generated from keel/slice.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_SLICE_CONST_CHAR_H
#define KEEL_KEEL_SLICE_CONST_CHAR_H
#include "keel/keel_slice_const_char.type.h"
#include "keel/keel_slice.type.h"
#include "keel/keel_outcome_keel_slice_const_char.type.h"

typedef struct keel_arena keel_arena;

#line 26 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_from(const char *p, size_t n);
#line 30 "keel/slice.k"
static inline size_t keel_slice_const_char_length(keel_slice_const_char s);
#line 31 "keel/slice.k"
static inline char    keel_slice_const_char_get (keel_slice_const_char s, size_t i);
#line 33 "keel/slice.k"
static inline const char   *keel_slice_const_char_ptr (keel_slice_const_char s);
#line 34 "keel/slice.k"
static inline const char   *keel_slice_const_char_ptr1(keel_slice_const_char s, size_t i);
#line 45 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_as_slice(keel_slice_const_char s, size_t a, size_t b);
#line 59 "keel/slice.k"
static inline keel_outcome_keel_slice_const_char keel_slice_const_char_clone(keel_arena *a, keel_slice_const_char s);
#line 68 "keel/slice.k"
static inline keel_slice_cursor keel_slice_const_char_begin(keel_slice_const_char s);
#line 69 "keel/slice.k"
static inline bool keel_slice_const_char_has_next(keel_slice_const_char s, keel_slice_cursor *c);
#line 70 "keel/slice.k"
static inline const char *keel_slice_const_char_next(keel_slice_const_char s, keel_slice_cursor *c);
#line 74 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_partition(keel_slice_const_char s, size_t k, size_t w);
#include "keel/keel_arena.h"
#include "keel/keel_outcome_keel_slice_const_char.h"

#line 26 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_from(const char *p, size_t n) {
    return (keel_slice_const_char){ p ? n : 0, p };
}
#line 30 "keel/slice.k"
static inline size_t keel_slice_const_char_length(keel_slice_const_char s) { return s.len; }
#line 31 "keel/slice.k"
static inline char    keel_slice_const_char_get (keel_slice_const_char s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return s.ptr[i]; }
#line 33 "keel/slice.k"
static inline const char   *keel_slice_const_char_ptr (keel_slice_const_char s) { return s.ptr; }
#line 34 "keel/slice.k"
static inline const char   *keel_slice_const_char_ptr1(keel_slice_const_char s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return &s.ptr[i]; }
#line 45 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_as_slice(keel_slice_const_char s, size_t a, size_t b) {
    KEEL_CHECK(a <= b && b <= keel_slice_const_char_length(s), "range-index-out-of-bounds");
    if (b > s.len) b = s.len;
    if (a > b) a = b;
    return (keel_slice_const_char){ b - a, s.ptr + a };
}
#line 59 "keel/slice.k"
static inline keel_outcome_keel_slice_const_char keel_slice_const_char_clone(keel_arena *a, keel_slice_const_char s) {
    keel_outcome_keel_slice_const_char r = {0};
    const char *data = (const char *)keel_arena_alloc2(a, sizeof(const char), _Alignof(const char), s.len);
    if (!data) return keel_outcome_keel_slice_const_char_none(&r);
    if (s.len > 0) memcpy((void *)data, s.ptr, s.len * sizeof(const char));
    return keel_outcome_keel_slice_const_char_win1(&r, (keel_slice_const_char){ s.len, data });
}
#line 68 "keel/slice.k"
static inline keel_slice_cursor keel_slice_const_char_begin(keel_slice_const_char s) { (void)s; return (keel_slice_cursor){0}; }
#line 69 "keel/slice.k"
static inline bool   keel_slice_const_char_has_next(keel_slice_const_char s, keel_slice_cursor *c) { return c->i < s.len; }
#line 70 "keel/slice.k"
static inline const char   *keel_slice_const_char_next(keel_slice_const_char s, keel_slice_cursor *c) { return &s.ptr[c->i++]; }
#line 74 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_partition(keel_slice_const_char s, size_t k, size_t w) {
    if (k == 0) return (keel_slice_const_char){0, s.ptr};
    size_t step = s.len / k + (s.len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= s.len) return (keel_slice_const_char){0, s.ptr};
    size_t hi = lo + step;
    if (hi > s.len) hi = s.len;
    return (keel_slice_const_char){ hi - lo, s.ptr + lo };
}
#endif /* KEEL_KEEL_SLICE_CONST_CHAR_H */
