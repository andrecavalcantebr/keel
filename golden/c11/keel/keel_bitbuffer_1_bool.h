/* keel/keel_bitbuffer_1_bool.h — generated from keel/bitbuffer.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BITBUFFER_1_BOOL_H
#define KEEL_KEEL_BITBUFFER_1_BOOL_H

#include "keel/keel_bitbuffer_1_bool.type.h"
#include "keel/keel_bitslice_1_bool.type.h"
#include "keel/keel_outcome_bool.type.h"
#include "keel/keel_outcome_keel_bitbuffer_1_bool.type.h"
#include "keel/keel_range.type.h"

typedef struct keel_arena keel_arena;
#line 20 "keel/bitbuffer.k"
static inline void keel_bitbuffer_1_bool_assert_instance(void);
#line 31 "keel/bitbuffer.k"
static inline keel_bitbuffer_1_bool keel_bitbuffer_1_bool_make(keel_arena *a, size_t cap);
#line 39 "keel/bitbuffer.k"
static inline keel_bitbuffer_1_bool keel_bitbuffer_1_bool_from(u8 *p, size_t cap);
#line 44 "keel/bitbuffer.k"
static inline keel_bitbuffer_1_bool keel_bitbuffer_1_bool_of(u8 *p, size_t n);
#line 48 "keel/bitbuffer.k"
static inline size_t keel_bitbuffer_1_bool_length(keel_bitbuffer_1_bool *b);
static inline size_t keel_bitbuffer_1_bool_capacity(keel_bitbuffer_1_bool *b);
static inline size_t keel_bitbuffer_1_bool_bytes(keel_bitbuffer_1_bool *b);
#line 52 "keel/bitbuffer.k"
static inline bool keel_bitbuffer_1_bool_get(keel_bitbuffer_1_bool *b, size_t i);
#line 57 "keel/bitbuffer.k"
static inline void keel_bitbuffer_1_bool_set(keel_bitbuffer_1_bool *b, size_t i, bool v);
#line 63 "keel/bitbuffer.k"
static inline bool keel_bitbuffer_1_bool_push(keel_bitbuffer_1_bool *b, bool v);
#line 71 "keel/bitbuffer.k"
static inline keel_outcome_bool keel_bitbuffer_1_bool_pop(keel_bitbuffer_1_bool *b);
#line 78 "keel/bitbuffer.k"
static inline void keel_bitbuffer_1_bool_clear(keel_bitbuffer_1_bool *b);
#line 81 "keel/bitbuffer.k"
static inline keel_outcome_bool keel_bitbuffer_1_bool_at(keel_bitbuffer_1_bool *b, size_t i);
#line 87 "keel/bitbuffer.k"
static inline keel_bitslice_1_bool keel_bitbuffer_1_bool_as_slice(keel_bitbuffer_1_bool *b);
#line 91 "keel/bitbuffer.k"
static inline keel_bitslice_1_bool keel_bitbuffer_1_bool_as_slice2(keel_bitbuffer_1_bool *b, size_t a, size_t c);
#line 98 "keel/bitbuffer.k"
static inline keel_bitslice_1_bool keel_bitbuffer_1_bool_as_slice1(keel_bitbuffer_1_bool *b, keel_range r);
#line 103 "keel/bitbuffer.k"
static inline keel_outcome_keel_bitbuffer_1_bool keel_bitbuffer_1_bool_clone(keel_arena *a, keel_bitbuffer_1_bool *b);
#line 113 "keel/bitbuffer.k"
static inline keel_bitslice_1_bool keel_bitbuffer_1_bool_partition(keel_bitbuffer_1_bool *b, size_t k, size_t w);

#include "keel/keel_arena.h"
#include "keel/keel_bits.h"
#include "keel/keel_bitslice_1_bool.h"
#include "keel/keel_outcome_bool.h"
#include "keel/keel_outcome_keel_bitbuffer_1_bool.h"

#line 20 "keel/bitbuffer.k"
static inline void keel_bitbuffer_1_bool_assert_instance(void) {
    static_assert(1 >= 1 && 1 <= 8 * sizeof(bool), "bitbuffer-width-invalid: W must be in 1..bits of T");
    static_assert((bool)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
    static_assert(!((bool)2 == 1) || 1 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
}
#line 31 "keel/bitbuffer.k"
static inline keel_bitbuffer_1_bool keel_bitbuffer_1_bool_make(keel_arena *a, size_t cap) {
    keel_bitbuffer_1_bool r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), _Alignof(u8), keel_bits_bytes(cap, 1));
    if (data) { r.cap = cap; r.words = data; }
    return r;
}
#line 39 "keel/bitbuffer.k"
static inline keel_bitbuffer_1_bool keel_bitbuffer_1_bool_from(u8 *p, size_t cap) {
    return (keel_bitbuffer_1_bool){ p ? cap : 0, 0, p };
}
#line 44 "keel/bitbuffer.k"
static inline keel_bitbuffer_1_bool keel_bitbuffer_1_bool_of(u8 *p, size_t n) {
    return (keel_bitbuffer_1_bool){ p ? n : 0, p ? n : 0, p };
}
#line 48 "keel/bitbuffer.k"
static inline size_t keel_bitbuffer_1_bool_length(keel_bitbuffer_1_bool *b) { return b->len; }
static inline size_t keel_bitbuffer_1_bool_capacity(keel_bitbuffer_1_bool *b) { return b->cap; }
static inline size_t keel_bitbuffer_1_bool_bytes(keel_bitbuffer_1_bool *b) { return keel_bits_bytes(b->cap, 1); }
#line 52 "keel/bitbuffer.k"
static inline bool keel_bitbuffer_1_bool_get(keel_bitbuffer_1_bool *b, size_t i) {
    KEEL_CHECK(i < b->len, "index-out-of-length");
    return (bool)keel_bits_read(b->words, i * 1, 1);
}
#line 57 "keel/bitbuffer.k"
static inline void keel_bitbuffer_1_bool_set(keel_bitbuffer_1_bool *b, size_t i, bool v) {
    KEEL_CHECK(i < b->len, "set-out-of-length");
    KEEL_CHECK(1 == 64 || ((u64)v >> (1 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(b->words, i * 1, 1, (u64)v);
}
#line 63 "keel/bitbuffer.k"
static inline bool keel_bitbuffer_1_bool_push(keel_bitbuffer_1_bool *b, bool v) {
    if (b->len == b->cap) return false;
    KEEL_CHECK(1 == 64 || ((u64)v >> (1 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(b->words, b->len * 1, 1, (u64)v);
    b->len++;
    return true;
}
#line 71 "keel/bitbuffer.k"
static inline keel_outcome_bool keel_bitbuffer_1_bool_pop(keel_bitbuffer_1_bool *b) {
    keel_outcome_bool r = {0};
    if (b->len == 0) return keel_outcome_bool_none(&r);
    b->len--;
    return keel_outcome_bool_win1(&r, (bool)keel_bits_read(b->words, b->len * 1, 1));
}
#line 78 "keel/bitbuffer.k"
static inline void keel_bitbuffer_1_bool_clear(keel_bitbuffer_1_bool *b) { b->len = 0; }
#line 81 "keel/bitbuffer.k"
static inline keel_outcome_bool keel_bitbuffer_1_bool_at(keel_bitbuffer_1_bool *b, size_t i) {
    keel_outcome_bool r = {0};
    return i < b->len ? keel_outcome_bool_win1(&r, (bool)keel_bits_read(b->words, i * 1, 1)) : keel_outcome_bool_none(&r);
}
#line 87 "keel/bitbuffer.k"
static inline keel_bitslice_1_bool keel_bitbuffer_1_bool_as_slice(keel_bitbuffer_1_bool *b) {
    return (keel_bitslice_1_bool){ b->len, 0, b->words };
}
#line 91 "keel/bitbuffer.k"
static inline keel_bitslice_1_bool keel_bitbuffer_1_bool_as_slice2(keel_bitbuffer_1_bool *b, size_t a, size_t c) {
#line 91 "keel/bitbuffer.k"
    KEEL_CHECK(a <= c && c <= keel_bitbuffer_1_bool_length(b), "range-index-out-of-bounds");
    if (c > b->len) c = b->len;
    if (a > c) a = c;
    size_t pos = a * 1;
    return (keel_bitslice_1_bool){ c - a, pos & 7, b->words + (pos >> 3) };
}
#line 98 "keel/bitbuffer.k"
static inline keel_bitslice_1_bool keel_bitbuffer_1_bool_as_slice1(keel_bitbuffer_1_bool *b, keel_range r) {
    return keel_bitbuffer_1_bool_as_slice2(b, r.first, r.limit);
}
#line 103 "keel/bitbuffer.k"
static inline keel_outcome_keel_bitbuffer_1_bool keel_bitbuffer_1_bool_clone(keel_arena *a, keel_bitbuffer_1_bool *b) {
    keel_outcome_keel_bitbuffer_1_bool r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), _Alignof(u8), keel_bits_bytes(b->cap, 1));
    if (!data) return keel_outcome_keel_bitbuffer_1_bool_none(&r);
    size_t n = keel_bits_bytes(b->len, 1);
    for (size_t i = 0; i < n; ++i) data[i] = b->words[i];
    return keel_outcome_keel_bitbuffer_1_bool_win1(&r, (keel_bitbuffer_1_bool){ b->cap, b->len, data });
}
#line 113 "keel/bitbuffer.k"
static inline keel_bitslice_1_bool keel_bitbuffer_1_bool_partition(keel_bitbuffer_1_bool *b, size_t k, size_t w) {
    return keel_bitslice_1_bool_partition(keel_bitbuffer_1_bool_as_slice(b), k, w);
}

#endif /* KEEL_KEEL_BITBUFFER_1_BOOL_H */
