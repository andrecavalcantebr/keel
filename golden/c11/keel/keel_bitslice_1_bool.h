/* keel/keel_bitslice_1_bool.h — generated from keel/bitslice.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BITSLICE_1_BOOL_H
#define KEEL_KEEL_BITSLICE_1_BOOL_H

#include "keel/keel_bitslice_1_bool.type.h"
#include "keel/keel_outcome_bool.type.h"
#include "keel/keel_outcome_keel_bitslice_1_bool.type.h"

typedef struct keel_arena keel_arena;
#line 20 "keel/bitslice.k"
static inline void keel_bitslice_1_bool_assert_instance(void);
#line 31 "keel/bitslice.k"
static inline keel_bitslice_1_bool keel_bitslice_1_bool_from(u8 *p, size_t n);
#line 35 "keel/bitslice.k"
static inline size_t keel_bitslice_1_bool_length(keel_bitslice_1_bool s);
#line 37 "keel/bitslice.k"
static inline bool keel_bitslice_1_bool_get(keel_bitslice_1_bool s, size_t i);
#line 42 "keel/bitslice.k"
static inline void keel_bitslice_1_bool_set(keel_bitslice_1_bool s, size_t i, bool v);
#line 49 "keel/bitslice.k"
static inline keel_outcome_bool keel_bitslice_1_bool_at(keel_bitslice_1_bool s, size_t i);
#line 55 "keel/bitslice.k"
static inline keel_bitslice_1_bool keel_bitslice_1_bool_as_slice(keel_bitslice_1_bool s, size_t a, size_t b);
#line 64 "keel/bitslice.k"
static inline keel_outcome_keel_bitslice_1_bool keel_bitslice_1_bool_clone(keel_arena *a, keel_bitslice_1_bool s);
#line 77 "keel/bitslice.k"
static inline keel_bitslice_1_bool keel_bitslice_1_bool_partition(keel_bitslice_1_bool s, size_t k, size_t w);

#include "keel/keel_arena.h"
#include "keel/keel_bits.h"
#include "keel/keel_outcome_bool.h"
#include "keel/keel_outcome_keel_bitslice_1_bool.h"

#line 20 "keel/bitslice.k"
static inline void keel_bitslice_1_bool_assert_instance(void) {
    static_assert(1 >= 1 && 1 <= 8 * sizeof(bool), "bitbuffer-width-invalid: W must be in 1..bits of T");
    static_assert((bool)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
    static_assert(!((bool)2 == 1) || 1 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
}
#line 31 "keel/bitslice.k"
static inline keel_bitslice_1_bool keel_bitslice_1_bool_from(u8 *p, size_t n) {
    return (keel_bitslice_1_bool){ p ? n : 0, 0, p };
}
#line 35 "keel/bitslice.k"
static inline size_t keel_bitslice_1_bool_length(keel_bitslice_1_bool s) { return s.len; }
#line 37 "keel/bitslice.k"
static inline bool keel_bitslice_1_bool_get(keel_bitslice_1_bool s, size_t i) {
    KEEL_CHECK(i < s.len, "index-out-of-length");
    return (bool)keel_bits_read(s.words, s.bit + i * 1, 1);
}
#line 42 "keel/bitslice.k"
static inline void keel_bitslice_1_bool_set(keel_bitslice_1_bool s, size_t i, bool v) {
    KEEL_CHECK(i < s.len, "set-out-of-length");
    KEEL_CHECK(1 == 64 || ((u64)v >> (1 % 64)) == 0, "bit-value-out-of-width");
    keel_bits_write(s.words, s.bit + i * 1, 1, (u64)v);
}
#line 49 "keel/bitslice.k"
static inline keel_outcome_bool keel_bitslice_1_bool_at(keel_bitslice_1_bool s, size_t i) {
    keel_outcome_bool r = {0};
    return i < s.len ? keel_outcome_bool_win1(&r, keel_bitslice_1_bool_get(s, i)) : keel_outcome_bool_none(&r);
}
#line 55 "keel/bitslice.k"
static inline keel_bitslice_1_bool keel_bitslice_1_bool_as_slice(keel_bitslice_1_bool s, size_t a, size_t b) {
#line 55 "keel/bitslice.k"
    KEEL_CHECK(a <= b && b <= keel_bitslice_1_bool_length(s), "range-index-out-of-bounds");
    if (b > s.len) b = s.len;
    if (a > b) a = b;
    size_t pos = s.bit + a * 1;
    return (keel_bitslice_1_bool){ b - a, pos & 7, s.words + (pos >> 3) };
}
#line 64 "keel/bitslice.k"
static inline keel_outcome_keel_bitslice_1_bool keel_bitslice_1_bool_clone(keel_arena *a, keel_bitslice_1_bool s) {
    keel_outcome_keel_bitslice_1_bool r = {0};
    u8 *data = (u8 *)keel_arena_alloc2(a, sizeof(u8), _Alignof(u8), keel_bits_bytes(s.len, 1));
    if (!data) return keel_outcome_keel_bitslice_1_bool_none(&r);
    for (size_t i = 0; i < s.len; ++i)
        keel_bits_write(data, i * 1, 1, keel_bits_read(s.words, s.bit + i * 1, 1));
    return keel_outcome_keel_bitslice_1_bool_win1(&r, (keel_bitslice_1_bool){ s.len, 0, data });
}
#line 77 "keel/bitslice.k"
static inline keel_bitslice_1_bool keel_bitslice_1_bool_partition(keel_bitslice_1_bool s, size_t k, size_t w) {
    if (k == 0) return (keel_bitslice_1_bool){ 0, s.bit, s.words };
    size_t g = keel_bits_group(1);
    size_t lead = keel_bits_lead(s.bit, 1);
    if (lead > s.len) lead = s.len;
    size_t core = s.len - lead;
    size_t step = core / k + (core % k ? 1 : 0);
    step = (step / g + (step % g ? 1 : 0)) * g;
    size_t lo = w == 0 ? 0 : lead + w * step;
    if (lo >= s.len) return (keel_bitslice_1_bool){ 0, s.bit, s.words };
    size_t hi = lead + (w + 1) * step;
    if (hi > s.len) hi = s.len;
    return keel_bitslice_1_bool_as_slice(s, lo, hi);
}

#endif /* KEEL_KEEL_BITSLICE_1_BOOL_H */
