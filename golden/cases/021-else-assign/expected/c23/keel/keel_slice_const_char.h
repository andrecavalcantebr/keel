/* keel/keel_slice_const_char.h — generated from keel/slice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_SLICE_CONST_CHAR_H
#define KEEL_KEEL_SLICE_CONST_CHAR_H
#include "keel/keel_slice_const_char.type.h"
#include "keel/keel_slice.type.h"

#line 25 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_from(const char *p, size_t n);
#line 29 "keel/slice.k"
static inline size_t keel_slice_const_char_length(keel_slice_const_char s);
#line 30 "keel/slice.k"
static inline char    keel_slice_const_char_get (keel_slice_const_char s, size_t i);
#line 32 "keel/slice.k"
static inline const char   *keel_slice_const_char_ptr (keel_slice_const_char s);
#line 33 "keel/slice.k"
static inline const char   *keel_slice_const_char_ptr1(keel_slice_const_char s, size_t i);
#line 43 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_of(keel_slice_const_char s, size_t a, size_t b);
#line 58 "keel/slice.k"
static inline keel_slice_cursor keel_slice_const_char_begin(keel_slice_const_char s);
#line 59 "keel/slice.k"
static inline bool keel_slice_const_char_has_next(keel_slice_const_char s, keel_slice_cursor *c);
#line 60 "keel/slice.k"
static inline const char *keel_slice_const_char_next(keel_slice_const_char s, keel_slice_cursor *c);
#line 64 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_partition(keel_slice_const_char s, size_t k, size_t w);

#line 25 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_from(const char *p, size_t n) {
    return (keel_slice_const_char){ p ? n : 0, p };
}
#line 29 "keel/slice.k"
static inline size_t keel_slice_const_char_length(keel_slice_const_char s) { return s.len; }
#line 30 "keel/slice.k"
static inline char    keel_slice_const_char_get (keel_slice_const_char s, size_t i) { return s.ptr[i]; }
#line 32 "keel/slice.k"
static inline const char   *keel_slice_const_char_ptr (keel_slice_const_char s) { return s.ptr; }
#line 33 "keel/slice.k"
static inline const char   *keel_slice_const_char_ptr1(keel_slice_const_char s, size_t i) { return &s.ptr[i]; }
#line 43 "keel/slice.k"
static inline keel_slice_const_char keel_slice_const_char_of(keel_slice_const_char s, size_t a, size_t b) {
    if (b > s.len) b = s.len; if (a > b) a = b;
    return (keel_slice_const_char){ b - a, s.ptr + a };
}
#line 58 "keel/slice.k"
static inline keel_slice_cursor keel_slice_const_char_begin(keel_slice_const_char s) { (void)s; return (keel_slice_cursor){0}; }
#line 59 "keel/slice.k"
static inline bool   keel_slice_const_char_has_next(keel_slice_const_char s, keel_slice_cursor *c) { return c->i < s.len; }
#line 60 "keel/slice.k"
static inline const char   *keel_slice_const_char_next(keel_slice_const_char s, keel_slice_cursor *c) { return &s.ptr[c->i++]; }
#line 64 "keel/slice.k"
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
