/* keel/keel_bitslice_33_u64.h — generated from keel/bitslice.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BITSLICE_33_U64_H
#define KEEL_KEEL_BITSLICE_33_U64_H

#include "keel/keel_bitslice_33_u64.type.h"
#include "keel/keel_outcome_u64.type.h"
#include "keel/keel_outcome_keel_bitslice_33_u64.type.h"

typedef struct keel_arena keel_arena;
#line 20 "keel/bitslice.k"
static inline void keel_bitslice_33_u64_assert_instance(void);
#line 31 "keel/bitslice.k"
static inline keel_bitslice_33_u64 keel_bitslice_33_u64_from(u8 *p, size_t n);
#line 35 "keel/bitslice.k"
static inline size_t keel_bitslice_33_u64_length(keel_bitslice_33_u64 s);
#line 37 "keel/bitslice.k"
static inline u64 keel_bitslice_33_u64_get(keel_bitslice_33_u64 s, size_t i);
#line 42 "keel/bitslice.k"
static inline void keel_bitslice_33_u64_set(keel_bitslice_33_u64 s, size_t i, u64 v);
#line 49 "keel/bitslice.k"
static inline keel_outcome_u64 keel_bitslice_33_u64_at(keel_bitslice_33_u64 s, size_t i);
#line 55 "keel/bitslice.k"
static inline keel_bitslice_33_u64 keel_bitslice_33_u64_as_slice(keel_bitslice_33_u64 s, size_t a, size_t b);
#line 64 "keel/bitslice.k"
static inline keel_outcome_keel_bitslice_33_u64 keel_bitslice_33_u64_clone(keel_arena *a, keel_bitslice_33_u64 s);
#line 77 "keel/bitslice.k"
static inline keel_bitslice_33_u64 keel_bitslice_33_u64_partition(keel_bitslice_33_u64 s, size_t k, size_t w);

#include "keel/keel_arena.h"
#include "keel/keel_bits.h"
#include "keel/keel_outcome_keel_bitslice_33_u64.h"
#include "keel/keel_outcome_u64.h"

#line 20 "keel/bitslice.k"
static inline void keel_bitslice_33_u64_assert_instance(void) {
    static_assert(33 >= 1 && 33 <= 8 * sizeof(u64), "bitbuffer-width-invalid: W must be in 1..bits of T");
    static_assert((u64)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
    static_assert(!((u64)2 == 1) || 33 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
}
#line 31 "keel/bitslice.k"
static inline keel_bitslice_33_u64 keel_bitslice_33_u64_from(u8 *p, size_t n) {
    return (keel_bitslice_33_u64){ p ? n : 0, 0, p };
}
#line 35 "keel/bitslice.k"
static inline size_t keel_bitslice_33_u64_length(keel_bitslice_33_u64 s) { return s.len; }
#line 37 "keel/bitslice.k"
static inline u64 keel_bitslice_33_u64_get(keel_bitslice_33_u64 s, size_t i) {
    KEEL_CHECK(i < s.len, "index-out-of-length");
    return (u64)keel_bits_read(s.words, s.bit + i * 33, 33);
}
#line 42 "keel/bitslice.k"
static inline void keel_bitslice_33_u64_set(keel_bitslice_33_u64 s, size_t i, u64 v) {
    KEEL_CHECK(i < s.len, "set-out-of-length");
    KEEL_CHECK(33 == 64 || ((u64)v >> (33 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(s.words, s.bit + i * 33, 33, (u64)v);
}
#line 49 "keel/bitslice.k"
static inline keel_outcome_u64 keel_bitslice_33_u64_at(keel_bitslice_33_u64 s, size_t i) {
    keel_outcome_u64 r = {0};
    return i < s.len ? keel_outcome_u64_win1(&r, keel_bitslice_33_u64_get(s, i)) : keel_outcome_u64_none(&r);
}
#line 55 "keel/bitslice.k"
static inline keel_bitslice_33_u64 keel_bitslice_33_u64_as_slice(keel_bitslice_33_u64 s, size_t a, size_t b) {
#line 55 "keel/bitslice.k"
    KEEL_CHECK(a <= b && b <= keel_bitslice_33_u64_length(s), "range-index-out-of-bounds");
    if (b > s.len) b = s.len;
    if (a > b) a = b;
    size_t pos = s.bit + a * 33;
    return (keel_bitslice_33_u64){ b - a, pos & 7, s.words + (pos >> 3) };
}
#line 64 "keel/bitslice.k"
static inline keel_outcome_keel_bitslice_33_u64 keel_bitslice_33_u64_clone(keel_arena *a, keel_bitslice_33_u64 s) {
    keel_outcome_keel_bitslice_33_u64 r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), _Alignof(u8), keel_bits_bytes(s.len, 33));
    if (!data) return keel_outcome_keel_bitslice_33_u64_none(&r);
    for (size_t i = 0; i < s.len; ++i)
        keel_bits_write(data, i * 33, 33, keel_bits_read(s.words, s.bit + i * 33, 33));
    return keel_outcome_keel_bitslice_33_u64_win1(&r, (keel_bitslice_33_u64){ s.len, 0, data });
}
#line 77 "keel/bitslice.k"
static inline keel_bitslice_33_u64 keel_bitslice_33_u64_partition(keel_bitslice_33_u64 s, size_t k, size_t w) {
    if (k == 0) return (keel_bitslice_33_u64){ 0, s.bit, s.words };
    size_t g = keel_bits_group(33);
    size_t lead = keel_bits_lead(s.bit, 33);
    if (lead > s.len) lead = s.len;
    size_t core = s.len - lead;
    size_t step = core / k + (core % k ? 1 : 0);
    step = (step / g + (step % g ? 1 : 0)) * g;
    size_t lo = w == 0 ? 0 : lead + w * step;
    if (lo >= s.len) return (keel_bitslice_33_u64){ 0, s.bit, s.words };
    size_t hi = lead + (w + 1) * step;
    if (hi > s.len) hi = s.len;
    return keel_bitslice_33_u64_as_slice(s, lo, hi);
}

#endif /* KEEL_KEEL_BITSLICE_33_U64_H */
