/* keel/keel_bitslice_13_u16.h — generated from keel/bitslice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_BITSLICE_13_U16_H
#define KEEL_KEEL_BITSLICE_13_U16_H

#include "keel/keel_bitslice_13_u16.type.h"
#include "keel/keel_outcome_u16.type.h"
#include "keel/keel_outcome_keel_bitslice_13_u16.type.h"

typedef struct keel_arena keel_arena;
#line 20 "keel/bitslice.k"
static inline void keel_bitslice_13_u16_assert_instance(void);
#line 31 "keel/bitslice.k"
static inline keel_bitslice_13_u16 keel_bitslice_13_u16_from(u8 *p, size_t n);
#line 35 "keel/bitslice.k"
static inline size_t keel_bitslice_13_u16_length(keel_bitslice_13_u16 s);
#line 37 "keel/bitslice.k"
static inline u16 keel_bitslice_13_u16_get(keel_bitslice_13_u16 s, size_t i);
#line 42 "keel/bitslice.k"
static inline void keel_bitslice_13_u16_set(keel_bitslice_13_u16 s, size_t i, u16 v);
#line 49 "keel/bitslice.k"
static inline keel_outcome_u16 keel_bitslice_13_u16_at(keel_bitslice_13_u16 s, size_t i);
#line 55 "keel/bitslice.k"
static inline keel_bitslice_13_u16 keel_bitslice_13_u16_as_slice(keel_bitslice_13_u16 s, size_t a, size_t b);
#line 64 "keel/bitslice.k"
static inline keel_outcome_keel_bitslice_13_u16 keel_bitslice_13_u16_clone(keel_arena *a, keel_bitslice_13_u16 s);
#line 77 "keel/bitslice.k"
static inline keel_bitslice_13_u16 keel_bitslice_13_u16_partition(keel_bitslice_13_u16 s, size_t k, size_t w);

#include "keel/keel_arena.h"
#include "keel/keel_bits.h"
#include "keel/keel_outcome_keel_bitslice_13_u16.h"
#include "keel/keel_outcome_u16.h"

#line 20 "keel/bitslice.k"
static inline void keel_bitslice_13_u16_assert_instance(void) {
    static_assert(13 >= 1 && 13 <= 8 * sizeof(u16), "bitbuffer-width-invalid: W must be in 1..bits of T");
    static_assert((u16)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
    static_assert(!((u16)2 == 1) || 13 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
}
#line 31 "keel/bitslice.k"
static inline keel_bitslice_13_u16 keel_bitslice_13_u16_from(u8 *p, size_t n) {
    return (keel_bitslice_13_u16){ p ? n : 0, 0, p };
}
#line 35 "keel/bitslice.k"
static inline size_t keel_bitslice_13_u16_length(keel_bitslice_13_u16 s) { return s.len; }
#line 37 "keel/bitslice.k"
static inline u16 keel_bitslice_13_u16_get(keel_bitslice_13_u16 s, size_t i) {
    KEEL_CHECK(i < s.len, "index-out-of-length");
    return (u16)keel_bits_read(s.words, s.bit + i * 13, 13);
}
#line 42 "keel/bitslice.k"
static inline void keel_bitslice_13_u16_set(keel_bitslice_13_u16 s, size_t i, u16 v) {
    KEEL_CHECK(i < s.len, "set-out-of-length");
    KEEL_CHECK(13 == 64 || ((u64)v >> (13 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(s.words, s.bit + i * 13, 13, (u64)v);
}
#line 49 "keel/bitslice.k"
static inline keel_outcome_u16 keel_bitslice_13_u16_at(keel_bitslice_13_u16 s, size_t i) {
    keel_outcome_u16 r = {0};
    return i < s.len ? keel_outcome_u16_win1(&r, keel_bitslice_13_u16_get(s, i)) : keel_outcome_u16_none(&r);
}
#line 55 "keel/bitslice.k"
static inline keel_bitslice_13_u16 keel_bitslice_13_u16_as_slice(keel_bitslice_13_u16 s, size_t a, size_t b) {
#line 55 "keel/bitslice.k"
    KEEL_CHECK(a <= b && b <= keel_bitslice_13_u16_length(s), "range-index-out-of-bounds");
    if (b > s.len) b = s.len;
    if (a > b) a = b;
    size_t pos = s.bit + a * 13;
    return (keel_bitslice_13_u16){ b - a, pos & 7, s.words + (pos >> 3) };
}
#line 64 "keel/bitslice.k"
static inline keel_outcome_keel_bitslice_13_u16 keel_bitslice_13_u16_clone(keel_arena *a, keel_bitslice_13_u16 s) {
    keel_outcome_keel_bitslice_13_u16 r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), alignof(u8), keel_bits_bytes(s.len, 13));
    if (!data) return keel_outcome_keel_bitslice_13_u16_none(&r);
    for (size_t i = 0; i < s.len; ++i)
        keel_bits_write(data, i * 13, 13, keel_bits_read(s.words, s.bit + i * 13, 13));
    return keel_outcome_keel_bitslice_13_u16_win1(&r, (keel_bitslice_13_u16){ s.len, 0, data });
}
#line 77 "keel/bitslice.k"
static inline keel_bitslice_13_u16 keel_bitslice_13_u16_partition(keel_bitslice_13_u16 s, size_t k, size_t w) {
    if (k == 0) return (keel_bitslice_13_u16){ 0, s.bit, s.words };
    size_t g = keel_bits_group(13);
    size_t lead = keel_bits_lead(s.bit, 13);
    if (lead > s.len) lead = s.len;
    size_t core = s.len - lead;
    size_t step = core / k + (core % k ? 1 : 0);
    step = (step / g + (step % g ? 1 : 0)) * g;
    size_t lo = w == 0 ? 0 : lead + w * step;
    if (lo >= s.len) return (keel_bitslice_13_u16){ 0, s.bit, s.words };
    size_t hi = lead + (w + 1) * step;
    if (hi > s.len) hi = s.len;
    return keel_bitslice_13_u16_as_slice(s, lo, hi);
}

#endif /* KEEL_KEEL_BITSLICE_13_U16_H */
