/* keel/keel_bits.h — generated from keel/bits.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_BITS_H
#define KEEL_KEEL_BITS_H

#include "keel/keel_bits.type.h"

#line 17 "keel/bits.k"
static inline u64 keel_bits_read(const u8 *w, size_t pos, unsigned width);
#line 31 "keel/bits.k"
static inline void keel_bits_write(u8 *w, size_t pos, unsigned width, u64 v);
#line 45 "keel/bits.k"
static inline size_t keel_bits_bytes(size_t n, unsigned width);
#line 51 "keel/bits.k"
static inline size_t keel_bits_group(unsigned width);
#line 59 "keel/bits.k"
static inline size_t keel_bits_lead(size_t bit, unsigned width);

#line 17 "keel/bits.k"
static inline u64 keel_bits_read(const u8 *w, size_t pos, unsigned width) {
    u64 v = 0;
    unsigned n = 0;
    while (n < width) {
        unsigned off = (unsigned)(pos & 7);
        unsigned take = 8 - off;
        if (take > width - n) take = width - n;
        v |= (u64)((w[pos >> 3] >> off) & ((1u << take) - 1u)) << n;
        n += take;
        pos += take;
    }
    return v;
}
#line 31 "keel/bits.k"
static inline void keel_bits_write(u8 *w, size_t pos, unsigned width, u64 v) {
    unsigned n = 0;
    while (n < width) {
        unsigned off = (unsigned)(pos & 7);
        unsigned take = 8 - off;
        if (take > width - n) take = width - n;
        unsigned mask = ((1u << take) - 1u) << off;
        w[pos >> 3] = (u8)((w[pos >> 3] & ~mask) | (((unsigned)(v >> n) << off) & mask));
        n += take;
        pos += take;
    }
}
#line 45 "keel/bits.k"
static inline size_t keel_bits_bytes(size_t n, unsigned width) {
    return (n / 8) * width + ((n % 8) * width + 7) / 8;
}
#line 51 "keel/bits.k"
static inline size_t keel_bits_group(unsigned width) {
    unsigned a = width, b = 8;
    while (b) { unsigned t = a % b; a = b; b = t; }
    return 8 / a;
}
#line 59 "keel/bits.k"
static inline size_t keel_bits_lead(size_t bit, unsigned width) {
    size_t e = 0;
    while (((bit + e * width) & 7) != 0) ++e;
    return e;
}

#endif /* KEEL_KEEL_BITS_H */
