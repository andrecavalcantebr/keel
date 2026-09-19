/* keel/keel_slice_f32.h — gerado de keel/slice.k pelo cgen, perfil C11. */
#ifndef KEEL_KEEL_SLICE_F32_H
#define KEEL_KEEL_SLICE_F32_H
#include "keel/keel_slice_f32.type.h"
#include "keel/keel_slice.type.h"

static inline keel_slice_f32 keel_slice_f32_from(f32 *p, size_t n);
static inline size_t keel_slice_f32_length(keel_slice_f32 s);
static inline f32    keel_slice_f32_get (keel_slice_f32 s, size_t i);
static inline f32   *keel_slice_f32_ptr (keel_slice_f32 s);
static inline f32   *keel_slice_f32_ptr1(keel_slice_f32 s, size_t i);
/* cursor e partição recebem a cópia, como todo verbo de `slice` */
static inline keel_slice_cursor keel_slice_f32_begin(keel_slice_f32 s);
static inline bool keel_slice_f32_has_next(keel_slice_f32 s, keel_slice_cursor *c);
static inline f32 *keel_slice_f32_next(keel_slice_f32 s, keel_slice_cursor *c);
static inline keel_slice_f32 keel_slice_f32_partition(keel_slice_f32 s, size_t k, size_t w);

static inline keel_slice_f32 keel_slice_f32_from(f32 *p, size_t n) { return (keel_slice_f32){ p ? n : 0, p }; }
static inline size_t keel_slice_f32_length(keel_slice_f32 s) { return s.len; }
static inline f32    keel_slice_f32_get (keel_slice_f32 s, size_t i) { return s.ptr[i]; }
static inline f32   *keel_slice_f32_ptr (keel_slice_f32 s) { return s.ptr; }
static inline f32   *keel_slice_f32_ptr1(keel_slice_f32 s, size_t i) { return &s.ptr[i]; }
/* cursor e partição recebem a cópia, como todo verbo de `slice` */
static inline keel_slice_cursor keel_slice_f32_begin(keel_slice_f32 s) {
    (void)s; return (keel_slice_cursor){ 0 };
}
static inline bool keel_slice_f32_has_next(keel_slice_f32 s, keel_slice_cursor *c) {
    return c->i < s.len;
}
static inline f32 *keel_slice_f32_next(keel_slice_f32 s, keel_slice_cursor *c) {
    return &s.ptr[c->i++];
}
static inline keel_slice_f32 keel_slice_f32_partition(keel_slice_f32 s, size_t k, size_t w) {
    size_t n = s.len, step = k ? (n + k - 1) / k : n;
    size_t lo = w * step, hi;
    if (lo > n) lo = n;
    hi = lo + step; if (hi > n) hi = n;
    return (keel_slice_f32){ hi - lo, s.ptr + lo };
}
#endif /* KEEL_KEEL_SLICE_F32_H */
