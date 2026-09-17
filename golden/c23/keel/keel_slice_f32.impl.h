/* keel/keel_slice_f32.impl.h — instância de `slice f32` (backend §5.2).
   `slice` NÃO é `byref`, então os acessores recebem CÓPIA, não ponteiro
   (linguagem §4.11). É o que faz `slice.length(slice.of(b))` aninhar sem
   temporário: `of` devolve valor e `length` consome valor. */
#ifndef KEEL_KEEL_SLICE_F32_IMPL_H
#define KEEL_KEEL_SLICE_F32_IMPL_H
#include "keel/keel_slice_f32.h"

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
    size_t n = s.len, passo = k ? (n + k - 1) / k : n;
    size_t lo = w * passo, hi;
    if (lo > n) lo = n;
    hi = lo + passo; if (hi > n) hi = n;
    return (keel_slice_f32){ hi - lo, s.ptr + lo };
}
#endif /* KEEL_KEEL_SLICE_F32_IMPL_H */
