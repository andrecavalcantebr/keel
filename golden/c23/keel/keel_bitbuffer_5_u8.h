/* keel/keel_bitbuffer_5_u8.h — generated from keel/bitbuffer.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_BITBUFFER_5_U8_H
#define KEEL_KEEL_BITBUFFER_5_U8_H

#include "keel/keel_bitbuffer_5_u8.type.h"
#include "keel/keel_bitslice_5_u8.type.h"
#include "keel/keel_outcome_u8.type.h"
#include "keel/keel_outcome_keel_bitbuffer_5_u8.type.h"
#include "keel/keel_range.type.h"

typedef struct keel_arena keel_arena;
#line 20 "keel/bitbuffer.k"
static inline void keel_bitbuffer_5_u8_assert_instance(void);
#line 31 "keel/bitbuffer.k"
static inline keel_bitbuffer_5_u8 keel_bitbuffer_5_u8_make(keel_arena *a, size_t cap);
#line 39 "keel/bitbuffer.k"
static inline keel_bitbuffer_5_u8 keel_bitbuffer_5_u8_from(u8 *p, size_t cap);
#line 44 "keel/bitbuffer.k"
static inline keel_bitbuffer_5_u8 keel_bitbuffer_5_u8_of(u8 *p, size_t n);
#line 48 "keel/bitbuffer.k"
static inline size_t keel_bitbuffer_5_u8_length(keel_bitbuffer_5_u8 *b);
static inline size_t keel_bitbuffer_5_u8_capacity(keel_bitbuffer_5_u8 *b);
static inline size_t keel_bitbuffer_5_u8_bytes(keel_bitbuffer_5_u8 *b);
#line 52 "keel/bitbuffer.k"
static inline u8 keel_bitbuffer_5_u8_get(keel_bitbuffer_5_u8 *b, size_t i);
#line 57 "keel/bitbuffer.k"
static inline void keel_bitbuffer_5_u8_set(keel_bitbuffer_5_u8 *b, size_t i, u8 v);
#line 63 "keel/bitbuffer.k"
static inline bool keel_bitbuffer_5_u8_push(keel_bitbuffer_5_u8 *b, u8 v);
#line 71 "keel/bitbuffer.k"
static inline keel_outcome_u8 keel_bitbuffer_5_u8_pop(keel_bitbuffer_5_u8 *b);
#line 78 "keel/bitbuffer.k"
static inline void keel_bitbuffer_5_u8_clear(keel_bitbuffer_5_u8 *b);
#line 81 "keel/bitbuffer.k"
static inline keel_outcome_u8 keel_bitbuffer_5_u8_at(keel_bitbuffer_5_u8 *b, size_t i);
#line 87 "keel/bitbuffer.k"
static inline keel_bitslice_5_u8 keel_bitbuffer_5_u8_as_slice(keel_bitbuffer_5_u8 *b);
#line 91 "keel/bitbuffer.k"
static inline keel_bitslice_5_u8 keel_bitbuffer_5_u8_as_slice2(keel_bitbuffer_5_u8 *b, size_t a, size_t c);
#line 98 "keel/bitbuffer.k"
static inline keel_bitslice_5_u8 keel_bitbuffer_5_u8_as_slice1(keel_bitbuffer_5_u8 *b, keel_range r);
#line 103 "keel/bitbuffer.k"
static inline keel_outcome_keel_bitbuffer_5_u8 keel_bitbuffer_5_u8_clone(keel_arena *a, keel_bitbuffer_5_u8 *b);
#line 113 "keel/bitbuffer.k"
static inline keel_bitslice_5_u8 keel_bitbuffer_5_u8_partition(keel_bitbuffer_5_u8 *b, size_t k, size_t w);

#include "keel/keel_arena.h"
#include "keel/keel_bits.h"
#include "keel/keel_bitslice_5_u8.h"
#include "keel/keel_outcome_keel_bitbuffer_5_u8.h"
#include "keel/keel_outcome_u8.h"

#line 20 "keel/bitbuffer.k"
static inline void keel_bitbuffer_5_u8_assert_instance(void) {
    static_assert(5 >= 1 && 5 <= 8 * sizeof(u8), "bitbuffer-width-invalid: W must be in 1..bits of T");
    static_assert((u8)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
    static_assert(!((u8)2 == 1) || 5 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
}
#line 31 "keel/bitbuffer.k"
static inline keel_bitbuffer_5_u8 keel_bitbuffer_5_u8_make(keel_arena *a, size_t cap) {
    keel_bitbuffer_5_u8 r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), alignof(u8), keel_bits_bytes(cap, 5));
    if (data) { r.cap = cap; r.words = data; }
    return r;
}
#line 39 "keel/bitbuffer.k"
static inline keel_bitbuffer_5_u8 keel_bitbuffer_5_u8_from(u8 *p, size_t cap) {
    return (keel_bitbuffer_5_u8){ p ? cap : 0, 0, p };
}
#line 44 "keel/bitbuffer.k"
static inline keel_bitbuffer_5_u8 keel_bitbuffer_5_u8_of(u8 *p, size_t n) {
    return (keel_bitbuffer_5_u8){ p ? n : 0, p ? n : 0, p };
}
#line 48 "keel/bitbuffer.k"
static inline size_t keel_bitbuffer_5_u8_length(keel_bitbuffer_5_u8 *b) { return b->len; }
static inline size_t keel_bitbuffer_5_u8_capacity(keel_bitbuffer_5_u8 *b) { return b->cap; }
static inline size_t keel_bitbuffer_5_u8_bytes(keel_bitbuffer_5_u8 *b) { return keel_bits_bytes(b->cap, 5); }
#line 52 "keel/bitbuffer.k"
static inline u8 keel_bitbuffer_5_u8_get(keel_bitbuffer_5_u8 *b, size_t i) {
    KEEL_CHECK(i < b->len, "index-out-of-length");
    return (u8)keel_bits_read(b->words, i * 5, 5);
}
#line 57 "keel/bitbuffer.k"
static inline void keel_bitbuffer_5_u8_set(keel_bitbuffer_5_u8 *b, size_t i, u8 v) {
    KEEL_CHECK(i < b->len, "set-out-of-length");
    KEEL_CHECK(5 == 64 || ((u64)v >> (5 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(b->words, i * 5, 5, (u64)v);
}
#line 63 "keel/bitbuffer.k"
static inline bool keel_bitbuffer_5_u8_push(keel_bitbuffer_5_u8 *b, u8 v) {
    if (b->len == b->cap) return false;
    KEEL_CHECK(5 == 64 || ((u64)v >> (5 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(b->words, b->len * 5, 5, (u64)v);
    b->len++;
    return true;
}
#line 71 "keel/bitbuffer.k"
static inline keel_outcome_u8 keel_bitbuffer_5_u8_pop(keel_bitbuffer_5_u8 *b) {
    keel_outcome_u8 r = {0};
    if (b->len == 0) return keel_outcome_u8_none(&r);
    b->len--;
    return keel_outcome_u8_win1(&r, (u8)keel_bits_read(b->words, b->len * 5, 5));
}
#line 78 "keel/bitbuffer.k"
static inline void keel_bitbuffer_5_u8_clear(keel_bitbuffer_5_u8 *b) { b->len = 0; }
#line 81 "keel/bitbuffer.k"
static inline keel_outcome_u8 keel_bitbuffer_5_u8_at(keel_bitbuffer_5_u8 *b, size_t i) {
    keel_outcome_u8 r = {0};
    return i < b->len ? keel_outcome_u8_win1(&r, (u8)keel_bits_read(b->words, i * 5, 5)) : keel_outcome_u8_none(&r);
}
#line 87 "keel/bitbuffer.k"
static inline keel_bitslice_5_u8 keel_bitbuffer_5_u8_as_slice(keel_bitbuffer_5_u8 *b) {
    return (keel_bitslice_5_u8){ b->len, 0, b->words };
}
#line 91 "keel/bitbuffer.k"
static inline keel_bitslice_5_u8 keel_bitbuffer_5_u8_as_slice2(keel_bitbuffer_5_u8 *b, size_t a, size_t c) {
#line 91 "keel/bitbuffer.k"
    KEEL_CHECK(a <= c && c <= keel_bitbuffer_5_u8_length(b), "range-index-out-of-bounds");
    if (c > b->len) c = b->len;
    if (a > c) a = c;
    size_t pos = a * 5;
    return (keel_bitslice_5_u8){ c - a, pos & 7, b->words + (pos >> 3) };
}
#line 98 "keel/bitbuffer.k"
static inline keel_bitslice_5_u8 keel_bitbuffer_5_u8_as_slice1(keel_bitbuffer_5_u8 *b, keel_range r) {
    return keel_bitbuffer_5_u8_as_slice2(b, r.first, r.limit);
}
#line 103 "keel/bitbuffer.k"
static inline keel_outcome_keel_bitbuffer_5_u8 keel_bitbuffer_5_u8_clone(keel_arena *a, keel_bitbuffer_5_u8 *b) {
    keel_outcome_keel_bitbuffer_5_u8 r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), alignof(u8), keel_bits_bytes(b->cap, 5));
    if (!data) return keel_outcome_keel_bitbuffer_5_u8_none(&r);
    size_t n = keel_bits_bytes(b->len, 5);
    for (size_t i = 0; i < n; ++i) data[i] = b->words[i];
    return keel_outcome_keel_bitbuffer_5_u8_win1(&r, (keel_bitbuffer_5_u8){ b->cap, b->len, data });
}
#line 113 "keel/bitbuffer.k"
static inline keel_bitslice_5_u8 keel_bitbuffer_5_u8_partition(keel_bitbuffer_5_u8 *b, size_t k, size_t w) {
    return keel_bitslice_5_u8_partition(keel_bitbuffer_5_u8_as_slice(b), k, w);
}

#endif /* KEEL_KEEL_BITBUFFER_5_U8_H */
