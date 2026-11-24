/* keel/keel_buffer_i64.h — generated from keel/buffer.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BUFFER_I64_H
#define KEEL_KEEL_BUFFER_I64_H

#include "keel/keel_buffer_i64.type.h"
#include "keel/keel_buffer.type.h"
#include "keel/keel_slice_i64.type.h"
#include "keel/keel_outcome_i64.type.h"
#include "keel/keel_outcome_keel_buffer_i64.type.h"
#include "keel/keel_range.type.h"

typedef struct keel_arena keel_arena;
#line 26 "keel/buffer.k"
static inline keel_buffer_i64 keel_buffer_i64_from(i64 *p, size_t n);
#line 31 "keel/buffer.k"
static inline keel_buffer_i64 keel_buffer_i64_of(i64 *p, size_t n);
#line 35 "keel/buffer.k"
static inline size_t keel_buffer_i64_length(keel_buffer_i64 *b);
static inline size_t keel_buffer_i64_capacity(keel_buffer_i64 *b);
static inline i64      keel_buffer_i64_get(keel_buffer_i64 *b, size_t i);
static inline void   keel_buffer_i64_set(keel_buffer_i64 *b, size_t i, i64 v);
static inline i64     *keel_buffer_i64_ptr(keel_buffer_i64 *b);
static inline i64     *keel_buffer_i64_ptr1(keel_buffer_i64 *b, size_t i);
#line 42 "keel/buffer.k"
static inline i64 *keel_buffer_i64_push(keel_buffer_i64 *b);
#line 47 "keel/buffer.k"
static inline i64 *keel_buffer_i64_push1(keel_buffer_i64 *b, i64 v);
#line 53 "keel/buffer.k"
static inline i64 *keel_buffer_i64_pop(keel_buffer_i64 *b);
#line 58 "keel/buffer.k"
static inline void keel_buffer_i64_clear(keel_buffer_i64 *b);
#line 61 "keel/buffer.k"
static inline keel_outcome_i64 keel_buffer_i64_at(keel_buffer_i64 *b, size_t i);
#line 69 "keel/buffer.k"
static inline keel_slice_i64 keel_buffer_i64_as_slice(keel_buffer_i64 *b);
#line 73 "keel/buffer.k"
static inline keel_slice_i64 keel_buffer_i64_as_slice2(keel_buffer_i64 *b, size_t a, size_t c);
#line 79 "keel/buffer.k"
static inline keel_slice_i64 keel_buffer_i64_as_slice1(keel_buffer_i64 *b, keel_range r);
#line 82 "keel/buffer.k"
static inline keel_outcome_keel_buffer_i64 keel_buffer_i64_clone(keel_arena *a, keel_buffer_i64 *b);
#line 92 "keel/buffer.k"
static inline keel_buffer_cursor keel_buffer_i64_begin(keel_buffer_i64 *b);
static inline bool   keel_buffer_i64_has_next(keel_buffer_i64 *b, keel_buffer_cursor *c);
static inline i64     *keel_buffer_i64_next(keel_buffer_i64 *b, keel_buffer_cursor *c);
#line 98 "keel/buffer.k"
static inline keel_slice_i64 keel_buffer_i64_partition(keel_buffer_i64 *b, size_t k, size_t w);

#include "keel/keel_arena.h"
#include "keel/keel_outcome_i64.h"
#include "keel/keel_outcome_keel_buffer_i64.h"

#line 26 "keel/buffer.k"
static inline keel_buffer_i64 keel_buffer_i64_from(i64 *p, size_t n) {
    return (keel_buffer_i64){ p ? n : 0, 0, p };
}
#line 31 "keel/buffer.k"
static inline keel_buffer_i64 keel_buffer_i64_of(i64 *p, size_t n) {
    return (keel_buffer_i64){ p ? n : 0, p ? n : 0, p };
}
#line 35 "keel/buffer.k"
static inline size_t keel_buffer_i64_length(keel_buffer_i64 *b) { return b->len; }
static inline size_t keel_buffer_i64_capacity(keel_buffer_i64 *b) { return b->cap; }
static inline i64      keel_buffer_i64_get(keel_buffer_i64 *b, size_t i) { KEEL_CHECK(i < b->len, "index-out-of-length"); return b->ptr[i]; }
static inline void   keel_buffer_i64_set(keel_buffer_i64 *b, size_t i, i64 v) { KEEL_CHECK(i < b->len, "set-out-of-length"); b->ptr[i] = v; }
static inline i64     *keel_buffer_i64_ptr(keel_buffer_i64 *b) { return b->ptr; }
static inline i64     *keel_buffer_i64_ptr1(keel_buffer_i64 *b, size_t i) { KEEL_CHECK(i < b->len, "index-out-of-length"); return &b->ptr[i]; }
#line 42 "keel/buffer.k"
static inline i64 *keel_buffer_i64_push(keel_buffer_i64 *b) {
    if (b->len == b->cap) return NULL;
    return &b->ptr[b->len++];
}
#line 47 "keel/buffer.k"
static inline i64 *keel_buffer_i64_push1(keel_buffer_i64 *b, i64 v) {
    if (b->len == b->cap) return NULL;
    b->ptr[b->len] = v;
    return &b->ptr[b->len++];
}
#line 53 "keel/buffer.k"
static inline i64 *keel_buffer_i64_pop(keel_buffer_i64 *b) {
    if (b->len == 0) return NULL;
    return &b->ptr[--b->len];
}
#line 58 "keel/buffer.k"
static inline void keel_buffer_i64_clear(keel_buffer_i64 *b) { b->len = 0; }
#line 61 "keel/buffer.k"
static inline keel_outcome_i64 keel_buffer_i64_at(keel_buffer_i64 *b, size_t i) {
    keel_outcome_i64 r = {0};
    return i < b->len ? keel_outcome_i64_win1(&r, b->ptr[i]) : keel_outcome_i64_none(&r);
}
#line 69 "keel/buffer.k"
static inline keel_slice_i64 keel_buffer_i64_as_slice(keel_buffer_i64 *b) {
    return (keel_slice_i64){ b->len, b->ptr };
}
#line 73 "keel/buffer.k"
static inline keel_slice_i64 keel_buffer_i64_as_slice2(keel_buffer_i64 *b, size_t a, size_t c) {
#line 73 "keel/buffer.k"
    KEEL_CHECK(a <= c && c <= keel_buffer_i64_length(b), "range-index-out-of-bounds");
    if (c > b->len) c = b->len; if (a > c) a = c;
    return (keel_slice_i64){ c - a, b->ptr + a };
}
#line 79 "keel/buffer.k"
static inline keel_slice_i64 keel_buffer_i64_as_slice1(keel_buffer_i64 *b, keel_range r) { return keel_buffer_i64_as_slice2(b, r.first, r.limit); }
#line 82 "keel/buffer.k"
static inline keel_outcome_keel_buffer_i64 keel_buffer_i64_clone(keel_arena *a, keel_buffer_i64 *b) {
    keel_outcome_keel_buffer_i64 r = {0};
    i64 *data = (i64 *)keel_arena_alloc2(a, sizeof(i64), _Alignof(i64), b->cap);
    if (!data) return keel_outcome_keel_buffer_i64_none(&r);
    if (b->len > 0) memcpy((void *)data, b->ptr, b->len * sizeof(i64));
    return keel_outcome_keel_buffer_i64_win1(&r, (keel_buffer_i64){ b->cap, b->len, data });
}
#line 92 "keel/buffer.k"
static inline keel_buffer_cursor keel_buffer_i64_begin(keel_buffer_i64 *b) { (void)b; return (keel_buffer_cursor){0}; }
static inline bool   keel_buffer_i64_has_next(keel_buffer_i64 *b, keel_buffer_cursor *c) { return c->i < b->len; }
static inline i64     *keel_buffer_i64_next(keel_buffer_i64 *b, keel_buffer_cursor *c) { return &b->ptr[c->i++]; }
#line 98 "keel/buffer.k"
static inline keel_slice_i64 keel_buffer_i64_partition(keel_buffer_i64 *b, size_t k, size_t w) {
    if (k == 0) return (keel_slice_i64){0, b->ptr};
    size_t step = b->len / k + (b->len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= b->len) return (keel_slice_i64){0, b->ptr};
    size_t hi = lo + step;
    if (hi > b->len) hi = b->len;
    return (keel_slice_i64){ hi - lo, b->ptr + lo };
}

#endif /* KEEL_KEEL_BUFFER_I64_H */
