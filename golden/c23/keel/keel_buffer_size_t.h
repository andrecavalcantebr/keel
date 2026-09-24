/* keel/keel_buffer_size_t.h — generated from keel/buffer.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_BUFFER_SIZE_T_H
#define KEEL_KEEL_BUFFER_SIZE_T_H
#include "keel/keel_buffer_size_t.type.h"
#include "keel/keel_buffer.type.h"
#include "keel/keel_slice_size_t.type.h"
#include "keel/keel_outcome_size_t.type.h"
#include "keel/keel_outcome_keel_buffer_size_t.type.h"
#include "keel/keel_range.type.h"

typedef struct keel_arena keel_arena;

#line 26 "keel/buffer.k"
static inline keel_buffer_size_t keel_buffer_size_t_from(size_t *p, size_t n);
#line 31 "keel/buffer.k"
static inline keel_buffer_size_t keel_buffer_size_t_of(size_t *p, size_t n);
#line 35 "keel/buffer.k"
static inline size_t keel_buffer_size_t_length  (keel_buffer_size_t *b);
#line 36 "keel/buffer.k"
static inline size_t keel_buffer_size_t_capacity(keel_buffer_size_t *b);
#line 37 "keel/buffer.k"
static inline size_t    keel_buffer_size_t_get (keel_buffer_size_t *b, size_t i);
#line 38 "keel/buffer.k"
static inline void   keel_buffer_size_t_set (keel_buffer_size_t *b, size_t i, size_t v);
#line 39 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_ptr (keel_buffer_size_t *b);
#line 40 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_ptr1(keel_buffer_size_t *b, size_t i);
#line 42 "keel/buffer.k"
[[nodiscard]]static inline size_t *keel_buffer_size_t_push (keel_buffer_size_t *b);
#line 47 "keel/buffer.k"
[[nodiscard]]static inline size_t *keel_buffer_size_t_push1(keel_buffer_size_t *b, size_t v);
#line 53 "keel/buffer.k"
[[nodiscard]]static inline size_t *keel_buffer_size_t_pop(keel_buffer_size_t *b);
#line 58 "keel/buffer.k"
static inline void keel_buffer_size_t_clear(keel_buffer_size_t *b);
#line 61 "keel/buffer.k"
static inline keel_outcome_size_t keel_buffer_size_t_at(keel_buffer_size_t *b, size_t i);
#line 69 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice (keel_buffer_size_t *b);
#line 73 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice2(keel_buffer_size_t *b, size_t a, size_t c);
#line 80 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice1(keel_buffer_size_t *b, keel_range r);
#line 83 "keel/buffer.k"
static inline keel_outcome_keel_buffer_size_t keel_buffer_size_t_clone(keel_arena *a, keel_buffer_size_t *b);
#line 93 "keel/buffer.k"
static inline keel_buffer_cursor keel_buffer_size_t_begin(keel_buffer_size_t *b);
#line 94 "keel/buffer.k"
static inline bool   keel_buffer_size_t_has_next(keel_buffer_size_t *b, keel_buffer_cursor *c);
#line 95 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_next(keel_buffer_size_t *b, keel_buffer_cursor *c);
#line 99 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_partition(keel_buffer_size_t *b, size_t k, size_t w);
#include "keel/keel_arena.h"
#include "keel/keel_outcome_size_t.h"
#include "keel/keel_outcome_keel_buffer_size_t.h"

#line 26 "keel/buffer.k"
static inline keel_buffer_size_t keel_buffer_size_t_from(size_t *p, size_t n) {
    return (keel_buffer_size_t){ p ? n : 0, 0, p };
}
#line 31 "keel/buffer.k"
static inline keel_buffer_size_t keel_buffer_size_t_of(size_t *p, size_t n) {
    return (keel_buffer_size_t){ p ? n : 0, p ? n : 0, p };
}
#line 35 "keel/buffer.k"
static inline size_t keel_buffer_size_t_length  (keel_buffer_size_t *b) { return b->len; }
#line 36 "keel/buffer.k"
static inline size_t keel_buffer_size_t_capacity(keel_buffer_size_t *b) { return b->cap; }
#line 37 "keel/buffer.k"
static inline size_t    keel_buffer_size_t_get (keel_buffer_size_t *b, size_t i) { KEEL_CHECK(i < b->len, "index-out-of-length"); return b->ptr[i]; }
#line 38 "keel/buffer.k"
static inline void   keel_buffer_size_t_set (keel_buffer_size_t *b, size_t i, size_t v) { KEEL_CHECK(i < b->len, "set-out-of-length"); b->ptr[i] = v; }
#line 39 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_ptr (keel_buffer_size_t *b) { return b->ptr; }
#line 40 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_ptr1(keel_buffer_size_t *b, size_t i) { KEEL_CHECK(i < b->len, "index-out-of-length"); return &b->ptr[i]; }
#line 42 "keel/buffer.k"
[[nodiscard]]static inline size_t *keel_buffer_size_t_push (keel_buffer_size_t *b) {
    if (b->len == b->cap) return NULL;
    return &b->ptr[b->len++];
}
#line 47 "keel/buffer.k"
[[nodiscard]]static inline size_t *keel_buffer_size_t_push1(keel_buffer_size_t *b, size_t v) {
    if (b->len == b->cap) return NULL;
    b->ptr[b->len] = v;
    return &b->ptr[b->len++];
}
#line 53 "keel/buffer.k"
[[nodiscard]]static inline size_t *keel_buffer_size_t_pop(keel_buffer_size_t *b) {
    if (b->len == 0) return NULL;
    return &b->ptr[--b->len];
}
#line 58 "keel/buffer.k"
static inline void keel_buffer_size_t_clear(keel_buffer_size_t *b) { b->len = 0; }
#line 61 "keel/buffer.k"
static inline keel_outcome_size_t keel_buffer_size_t_at(keel_buffer_size_t *b, size_t i) {
    keel_outcome_size_t r = {0};
    return i < b->len ? keel_outcome_size_t_win1(&r, b->ptr[i]) : keel_outcome_size_t_none(&r);
}
#line 69 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice (keel_buffer_size_t *b) {
    return (keel_slice_size_t){ b->len, b->ptr };
}
#line 73 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice2(keel_buffer_size_t *b, size_t a, size_t c) {
    KEEL_CHECK(a <= c && c <= b->len, "range-index-out-of-bounds");
    if (c > b->len) c = b->len; if (a > c) a = c;
    return (keel_slice_size_t){ c - a, b->ptr + a };
}
#line 80 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice1(keel_buffer_size_t *b, keel_range r) { return keel_buffer_size_t_as_slice2(b, r.first, r.limit); }
#line 83 "keel/buffer.k"
static inline keel_outcome_keel_buffer_size_t keel_buffer_size_t_clone(keel_arena *a, keel_buffer_size_t *b) {
    keel_outcome_keel_buffer_size_t r = {0};
    size_t *data = (size_t *)keel_arena_alloc2(a, sizeof(size_t), alignof(size_t), b->cap);
    if (!data) return keel_outcome_keel_buffer_size_t_none(&r);
    if (b->len > 0) memcpy(data, b->ptr, b->len * sizeof(size_t));
    return keel_outcome_keel_buffer_size_t_win1(&r, (keel_buffer_size_t){ b->cap, b->len, data });
}
#line 93 "keel/buffer.k"
static inline keel_buffer_cursor keel_buffer_size_t_begin(keel_buffer_size_t *b) { (void)b; return (keel_buffer_cursor){0}; }
#line 94 "keel/buffer.k"
static inline bool   keel_buffer_size_t_has_next(keel_buffer_size_t *b, keel_buffer_cursor *c) { return c->i < b->len; }
#line 95 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_next(keel_buffer_size_t *b, keel_buffer_cursor *c) { return &b->ptr[c->i++]; }
#line 99 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_partition(keel_buffer_size_t *b, size_t k, size_t w) {
    if (k == 0) return (keel_slice_size_t){0, b->ptr};
    size_t step = b->len / k + (b->len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= b->len) return (keel_slice_size_t){0, b->ptr};
    size_t hi = lo + step;
    if (hi > b->len) hi = b->len;
    return (keel_slice_size_t){ hi - lo, b->ptr + lo };
}
#endif /* KEEL_KEEL_BUFFER_SIZE_T_H */
