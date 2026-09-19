/* keel/keel_slice_i32.h — generated from keel/slice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_SLICE_I32_H
#define KEEL_KEEL_SLICE_I32_H
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_slice.type.h"

static inline keel_slice_i32 keel_slice_i32_from(i32 *p, size_t n);
static inline size_t keel_slice_i32_length(keel_slice_i32 s);
static inline i32    keel_slice_i32_get (keel_slice_i32 s, size_t i);
static inline i32   *keel_slice_i32_ptr (keel_slice_i32 s);
static inline i32   *keel_slice_i32_ptr1(keel_slice_i32 s, size_t i);
/* cursor e partição recebem a cópia, como todo verbo de `slice` */
static inline keel_slice_cursor keel_slice_i32_begin(keel_slice_i32 s);
static inline bool keel_slice_i32_has_next(keel_slice_i32 s, keel_slice_cursor *c);
static inline i32 *keel_slice_i32_next(keel_slice_i32 s, keel_slice_cursor *c);
static inline keel_slice_i32 keel_slice_i32_partition(keel_slice_i32 s, size_t k, size_t w);

static inline keel_slice_i32 keel_slice_i32_from(i32 *p, size_t n) { return (keel_slice_i32){ p ? n : 0, p }; }
static inline size_t keel_slice_i32_length(keel_slice_i32 s) { return s.len; }
static inline i32    keel_slice_i32_get (keel_slice_i32 s, size_t i) { return s.ptr[i]; }
static inline i32   *keel_slice_i32_ptr (keel_slice_i32 s) { return s.ptr; }
static inline i32   *keel_slice_i32_ptr1(keel_slice_i32 s, size_t i) { return &s.ptr[i]; }
/* cursor e partição recebem a cópia, como todo verbo de `slice` */
static inline keel_slice_cursor keel_slice_i32_begin(keel_slice_i32 s) {
    (void)s; return (keel_slice_cursor){ 0 };
}
static inline bool keel_slice_i32_has_next(keel_slice_i32 s, keel_slice_cursor *c) {
    return c->i < s.len;
}
static inline i32 *keel_slice_i32_next(keel_slice_i32 s, keel_slice_cursor *c) {
    return &s.ptr[c->i++];
}
static inline keel_slice_i32 keel_slice_i32_partition(keel_slice_i32 s, size_t k, size_t w) {
    size_t n = s.len, step = k ? (n + k - 1) / k : n;
    size_t lo = w * step, hi;
    if (lo > n) lo = n;
    hi = lo + step; if (hi > n) hi = n;
    return (keel_slice_i32){ hi - lo, s.ptr + lo };
}
#endif /* KEEL_KEEL_SLICE_I32_H */
