/*
 * DESC: keel bits — packing of W-bit fields in a byte array (spec §5.8, backend §5.20)
 * AUTHOR: andre cavalcante
 * DATE: Nov, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.bits;

// The packing that `bitbuffer` and `bitslice` share. A field of `width` bits
// (1..64) at bit position `pos`: bits are numbered from the least significant
// bit of byte 0, and the access is by byte, so the result does not depend on
// the host's endianness (spec §5.8, layout). It is a module of its own because
// it mentions neither `W` nor `T`, and a generic module's fixed part cannot
// hold a function an instance calls (backend §5.20).

pub inline u64 read(const u8 *w, size_t pos, unsigned width) {
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

pub inline void write(u8 *w, size_t pos, unsigned width, u64 v) {
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

// bytes that hold `n` fields of `width` bits, without overflowing `n * width`.
pub inline size_t bytes(size_t n, unsigned width) {
    return (n / 8) * width + ((n % 8) * width + 7) / 8;
}

// fields per group that ends on a byte boundary: 8 / gcd(width, 8) (spec §5.8,
// rule 4). The part boundaries of `partition` are multiples of it.
pub inline size_t group(unsigned width) {
    unsigned a = width, b = 8;
    while (b) { unsigned t = a % b; a = b; b = t; }
    return 8 / a;
}

// fields before the first byte boundary: the smallest `e` such that field `e`
// of a run that starts at bit `bit` starts a byte.
pub inline size_t lead(size_t bit, unsigned width) {
    size_t e = 0;
    while (((bit + e * width) & 7) != 0) ++e;
    return e;
}
