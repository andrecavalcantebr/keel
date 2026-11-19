/* keel/keel_bitbuffer_33_u64.h — generated from keel/bitbuffer.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_BITBUFFER_33_U64_H
#define KEEL_KEEL_BITBUFFER_33_U64_H

#include "keel/keel_bitbuffer_33_u64.type.h"
#include "keel/keel_bitslice_33_u64.type.h"
#include "keel/keel_outcome_u64.type.h"
#include "keel/keel_outcome_keel_bitbuffer_33_u64.type.h"
#include "keel/keel_range.type.h"

typedef struct keel_arena keel_arena;
#line 30 "keel/bitbuffer.k"
static inline keel_bitbuffer_33_u64 keel_bitbuffer_33_u64_make(keel_arena *a, size_t cap);
#line 38 "keel/bitbuffer.k"
static inline keel_bitbuffer_33_u64 keel_bitbuffer_33_u64_from(u8 *p, size_t cap);
#line 43 "keel/bitbuffer.k"
static inline keel_bitbuffer_33_u64 keel_bitbuffer_33_u64_of(u8 *p, size_t n);
#line 47 "keel/bitbuffer.k"
static inline size_t keel_bitbuffer_33_u64_length(keel_bitbuffer_33_u64 *b);
static inline size_t keel_bitbuffer_33_u64_capacity(keel_bitbuffer_33_u64 *b);
static inline size_t keel_bitbuffer_33_u64_bytes(keel_bitbuffer_33_u64 *b);
#line 51 "keel/bitbuffer.k"
static inline u64 keel_bitbuffer_33_u64_get(keel_bitbuffer_33_u64 *b, size_t i);
#line 56 "keel/bitbuffer.k"
static inline void keel_bitbuffer_33_u64_set(keel_bitbuffer_33_u64 *b, size_t i, u64 v);
#line 62 "keel/bitbuffer.k"
static inline bool keel_bitbuffer_33_u64_push(keel_bitbuffer_33_u64 *b, u64 v);
#line 70 "keel/bitbuffer.k"
static inline keel_outcome_u64 keel_bitbuffer_33_u64_pop(keel_bitbuffer_33_u64 *b);
#line 77 "keel/bitbuffer.k"
static inline void keel_bitbuffer_33_u64_clear(keel_bitbuffer_33_u64 *b);
#line 80 "keel/bitbuffer.k"
static inline keel_outcome_u64 keel_bitbuffer_33_u64_at(keel_bitbuffer_33_u64 *b, size_t i);
#line 86 "keel/bitbuffer.k"
static inline keel_bitslice_33_u64 keel_bitbuffer_33_u64_as_slice(keel_bitbuffer_33_u64 *b);
#line 90 "keel/bitbuffer.k"
static inline keel_bitslice_33_u64 keel_bitbuffer_33_u64_as_slice2(keel_bitbuffer_33_u64 *b, size_t a, size_t c);
#line 97 "keel/bitbuffer.k"
static inline keel_bitslice_33_u64 keel_bitbuffer_33_u64_as_slice1(keel_bitbuffer_33_u64 *b, keel_range r);
#line 102 "keel/bitbuffer.k"
static inline keel_outcome_keel_bitbuffer_33_u64 keel_bitbuffer_33_u64_clone(keel_arena *a, keel_bitbuffer_33_u64 *b);
#line 112 "keel/bitbuffer.k"
static inline keel_bitslice_33_u64 keel_bitbuffer_33_u64_partition(keel_bitbuffer_33_u64 *b, size_t k, size_t w);

#include "keel/keel_arena.h"
#include "keel/keel_bits.h"
#include "keel/keel_bitslice_33_u64.h"
#include "keel/keel_outcome_keel_bitbuffer_33_u64.h"
#include "keel/keel_outcome_u64.h"

#line 30 "keel/bitbuffer.k"
static inline keel_bitbuffer_33_u64 keel_bitbuffer_33_u64_make(keel_arena *a, size_t cap) {
    keel_bitbuffer_33_u64 r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), alignof(u8), keel_bits_bytes(cap, 33));
    if (data) { r.cap = cap; r.words = data; }
    return r;
}
#line 38 "keel/bitbuffer.k"
static inline keel_bitbuffer_33_u64 keel_bitbuffer_33_u64_from(u8 *p, size_t cap) {
    return (keel_bitbuffer_33_u64){ p ? cap : 0, 0, p };
}
#line 43 "keel/bitbuffer.k"
static inline keel_bitbuffer_33_u64 keel_bitbuffer_33_u64_of(u8 *p, size_t n) {
    return (keel_bitbuffer_33_u64){ p ? n : 0, p ? n : 0, p };
}
#line 47 "keel/bitbuffer.k"
static inline size_t keel_bitbuffer_33_u64_length(keel_bitbuffer_33_u64 *b) { return b->len; }
static inline size_t keel_bitbuffer_33_u64_capacity(keel_bitbuffer_33_u64 *b) { return b->cap; }
static inline size_t keel_bitbuffer_33_u64_bytes(keel_bitbuffer_33_u64 *b) { return keel_bits_bytes(b->cap, 33); }
#line 51 "keel/bitbuffer.k"
static inline u64 keel_bitbuffer_33_u64_get(keel_bitbuffer_33_u64 *b, size_t i) {
    KEEL_CHECK(i < b->len, "index-out-of-length");
    return (u64)keel_bits_read(b->words, i * 33, 33);
}
#line 56 "keel/bitbuffer.k"
static inline void keel_bitbuffer_33_u64_set(keel_bitbuffer_33_u64 *b, size_t i, u64 v) {
    KEEL_CHECK(i < b->len, "set-out-of-length");
    KEEL_CHECK(33 == 64 || ((u64)v >> (33 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(b->words, i * 33, 33, (u64)v);
}
#line 62 "keel/bitbuffer.k"
static inline bool keel_bitbuffer_33_u64_push(keel_bitbuffer_33_u64 *b, u64 v) {
    if (b->len == b->cap) return false;
    KEEL_CHECK(33 == 64 || ((u64)v >> (33 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(b->words, b->len * 33, 33, (u64)v);
    b->len++;
    return true;
}
#line 70 "keel/bitbuffer.k"
static inline keel_outcome_u64 keel_bitbuffer_33_u64_pop(keel_bitbuffer_33_u64 *b) {
    keel_outcome_u64 r = {0};
    if (b->len == 0) return keel_outcome_u64_none(&r);
    b->len--;
    return keel_outcome_u64_win1(&r, (u64)keel_bits_read(b->words, b->len * 33, 33));
}
#line 77 "keel/bitbuffer.k"
static inline void keel_bitbuffer_33_u64_clear(keel_bitbuffer_33_u64 *b) { b->len = 0; }
#line 80 "keel/bitbuffer.k"
static inline keel_outcome_u64 keel_bitbuffer_33_u64_at(keel_bitbuffer_33_u64 *b, size_t i) {
    keel_outcome_u64 r = {0};
    return i < b->len ? keel_outcome_u64_win1(&r, (u64)keel_bits_read(b->words, i * 33, 33)) : keel_outcome_u64_none(&r);
}
#line 86 "keel/bitbuffer.k"
static inline keel_bitslice_33_u64 keel_bitbuffer_33_u64_as_slice(keel_bitbuffer_33_u64 *b) {
    return (keel_bitslice_33_u64){ b->len, 0, b->words };
}
#line 90 "keel/bitbuffer.k"
static inline keel_bitslice_33_u64 keel_bitbuffer_33_u64_as_slice2(keel_bitbuffer_33_u64 *b, size_t a, size_t c) {
#line 90 "keel/bitbuffer.k"
    KEEL_CHECK(a <= c && c <= keel_bitbuffer_33_u64_length(b), "range-index-out-of-bounds");
    if (c > b->len) c = b->len;
    if (a > c) a = c;
    size_t pos = a * 33;
    return (keel_bitslice_33_u64){ c - a, pos & 7, b->words + (pos >> 3) };
}
#line 97 "keel/bitbuffer.k"
static inline keel_bitslice_33_u64 keel_bitbuffer_33_u64_as_slice1(keel_bitbuffer_33_u64 *b, keel_range r) {
    return keel_bitbuffer_33_u64_as_slice2(b, r.first, r.limit);
}
#line 102 "keel/bitbuffer.k"
static inline keel_outcome_keel_bitbuffer_33_u64 keel_bitbuffer_33_u64_clone(keel_arena *a, keel_bitbuffer_33_u64 *b) {
    keel_outcome_keel_bitbuffer_33_u64 r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), alignof(u8), keel_bits_bytes(b->cap, 33));
    if (!data) return keel_outcome_keel_bitbuffer_33_u64_none(&r);
    size_t n = keel_bits_bytes(b->len, 33);
    for (size_t i = 0; i < n; ++i) data[i] = b->words[i];
    return keel_outcome_keel_bitbuffer_33_u64_win1(&r, (keel_bitbuffer_33_u64){ b->cap, b->len, data });
}
#line 112 "keel/bitbuffer.k"
static inline keel_bitslice_33_u64 keel_bitbuffer_33_u64_partition(keel_bitbuffer_33_u64 *b, size_t k, size_t w) {
    return keel_bitslice_33_u64_partition(keel_bitbuffer_33_u64_as_slice(b), k, w);
}

#endif /* KEEL_KEEL_BITBUFFER_33_U64_H */
