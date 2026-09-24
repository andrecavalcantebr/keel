/* keel/keel_buffer_size_t.h — generated from keel/buffer.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BUFFER_SIZE_T_H
#define KEEL_KEEL_BUFFER_SIZE_T_H
#include "keel/keel_buffer_size_t.type.h"
#include "keel/keel_buffer.type.h"
#include "keel/keel_slice_size_t.type.h"
#include "keel/keel_outcome_size_t.type.h"
#include "keel/keel_outcome_keel_buffer_size_t.type.h"

typedef struct keel_arena keel_arena;

#line 25 "keel/buffer.k"
static inline keel_buffer_size_t keel_buffer_size_t_from(size_t *p, size_t n);
#line 30 "keel/buffer.k"
static inline keel_buffer_size_t keel_buffer_size_t_of(size_t *p, size_t n);
#line 34 "keel/buffer.k"
static inline size_t keel_buffer_size_t_length  (keel_buffer_size_t *b);
#line 35 "keel/buffer.k"
static inline size_t keel_buffer_size_t_capacity(keel_buffer_size_t *b);
#line 36 "keel/buffer.k"
static inline size_t    keel_buffer_size_t_get (keel_buffer_size_t *b, size_t i);
#line 37 "keel/buffer.k"
static inline void   keel_buffer_size_t_set (keel_buffer_size_t *b, size_t i, size_t v);
#line 38 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_ptr (keel_buffer_size_t *b);
#line 39 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_ptr1(keel_buffer_size_t *b, size_t i);
#line 41 "keel/buffer.k"
static inline size_t *keel_buffer_size_t_push (keel_buffer_size_t *b);
#line 46 "keel/buffer.k"
static inline size_t *keel_buffer_size_t_push1(keel_buffer_size_t *b, size_t v);
#line 52 "keel/buffer.k"
static inline size_t *keel_buffer_size_t_pop(keel_buffer_size_t *b);
#line 57 "keel/buffer.k"
static inline void keel_buffer_size_t_clear(keel_buffer_size_t *b);
#line 60 "keel/buffer.k"
static inline keel_outcome_size_t keel_buffer_size_t_at(keel_buffer_size_t *b, size_t i);
#line 68 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice (keel_buffer_size_t *b);
#line 72 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice2(keel_buffer_size_t *b, size_t a, size_t c);
#line 78 "keel/buffer.k"
static inline keel_outcome_keel_buffer_size_t keel_buffer_size_t_clone(keel_arena *a, keel_buffer_size_t *b);
#line 88 "keel/buffer.k"
static inline keel_buffer_cursor keel_buffer_size_t_begin(keel_buffer_size_t *b);
#line 89 "keel/buffer.k"
static inline bool   keel_buffer_size_t_has_next(keel_buffer_size_t *b, keel_buffer_cursor *c);
#line 90 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_next(keel_buffer_size_t *b, keel_buffer_cursor *c);
#line 94 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_partition(keel_buffer_size_t *b, size_t k, size_t w);
#include "keel/keel_arena.h"
#include "keel/keel_outcome_size_t.h"
#include "keel/keel_outcome_keel_buffer_size_t.h"

#line 25 "keel/buffer.k"
static inline keel_buffer_size_t keel_buffer_size_t_from(size_t *p, size_t n) {
    return (keel_buffer_size_t){ p ? n : 0, 0, p };
}
#line 30 "keel/buffer.k"
static inline keel_buffer_size_t keel_buffer_size_t_of(size_t *p, size_t n) {
    return (keel_buffer_size_t){ p ? n : 0, p ? n : 0, p };
}
#line 34 "keel/buffer.k"
static inline size_t keel_buffer_size_t_length  (keel_buffer_size_t *b) { return b->len; }
#line 35 "keel/buffer.k"
static inline size_t keel_buffer_size_t_capacity(keel_buffer_size_t *b) { return b->cap; }
#line 36 "keel/buffer.k"
static inline size_t    keel_buffer_size_t_get (keel_buffer_size_t *b, size_t i) { return b->ptr[i]; }
#line 37 "keel/buffer.k"
static inline void   keel_buffer_size_t_set (keel_buffer_size_t *b, size_t i, size_t v) { b->ptr[i] = v; }
#line 38 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_ptr (keel_buffer_size_t *b) { return b->ptr; }
#line 39 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_ptr1(keel_buffer_size_t *b, size_t i) { return &b->ptr[i]; }
#line 41 "keel/buffer.k"
static inline size_t *keel_buffer_size_t_push (keel_buffer_size_t *b) {
    if (b->len == b->cap) return NULL;
    return &b->ptr[b->len++];
}
#line 46 "keel/buffer.k"
static inline size_t *keel_buffer_size_t_push1(keel_buffer_size_t *b, size_t v) {
    if (b->len == b->cap) return NULL;
    b->ptr[b->len] = v;
    return &b->ptr[b->len++];
}
#line 52 "keel/buffer.k"
static inline size_t *keel_buffer_size_t_pop(keel_buffer_size_t *b) {
    if (b->len == 0) return NULL;
    return &b->ptr[--b->len];
}
#line 57 "keel/buffer.k"
static inline void keel_buffer_size_t_clear(keel_buffer_size_t *b) { b->len = 0; }
#line 60 "keel/buffer.k"
static inline keel_outcome_size_t keel_buffer_size_t_at(keel_buffer_size_t *b, size_t i) {
    keel_outcome_size_t r = {0};
    return i < b->len ? keel_outcome_size_t_win1(&r, b->ptr[i]) : keel_outcome_size_t_none(&r);
}
#line 68 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice (keel_buffer_size_t *b) {
    return (keel_slice_size_t){ b->len, b->ptr };
}
#line 72 "keel/buffer.k"
static inline keel_slice_size_t keel_buffer_size_t_as_slice2(keel_buffer_size_t *b, size_t a, size_t c) {
    if (c > b->len) c = b->len; if (a > c) a = c;
    return (keel_slice_size_t){ c - a, b->ptr + a };
}
#line 78 "keel/buffer.k"
static inline keel_outcome_keel_buffer_size_t keel_buffer_size_t_clone(keel_arena *a, keel_buffer_size_t *b) {
    keel_outcome_keel_buffer_size_t r = {0};
    size_t *data = (size_t *)keel_arena_alloc2(a, sizeof(size_t), _Alignof(size_t), b->cap);
    if (!data) return keel_outcome_keel_buffer_size_t_none(&r);
    if (b->len > 0) memcpy(data, b->ptr, b->len * sizeof(size_t));
    return keel_outcome_keel_buffer_size_t_win1(&r, (keel_buffer_size_t){ b->cap, b->len, data });
}
#line 88 "keel/buffer.k"
static inline keel_buffer_cursor keel_buffer_size_t_begin(keel_buffer_size_t *b) { (void)b; return (keel_buffer_cursor){0}; }
#line 89 "keel/buffer.k"
static inline bool   keel_buffer_size_t_has_next(keel_buffer_size_t *b, keel_buffer_cursor *c) { return c->i < b->len; }
#line 90 "keel/buffer.k"
static inline size_t   *keel_buffer_size_t_next(keel_buffer_size_t *b, keel_buffer_cursor *c) { return &b->ptr[c->i++]; }
#line 94 "keel/buffer.k"
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
