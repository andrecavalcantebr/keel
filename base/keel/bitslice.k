/*
 * DESC: keel bitslice — fixed-extent view over W-bit elements (spec §5.8, backend §5.20)
 * AUTHOR: andre cavalcante
 * DATE: Nov, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.bitslice dim W type T;

import keel.arena   as arena   types;
import keel.outcome as outcome types;
import keel.bits    as bits;

// the widths and the element type are checked once per instance (spec §5.8,
// rule 1): W bits must fit in T, T is unsigned or `bool`, and a `bool` holds
// one bit. `(T)-1 > 0` is false for a signed type and for a float. The module
// writes the checks once, and every instance carries them with `W` and `T`
// substituted, so they fail the translation unit that uses a bad instance.
static_assert(W >= 1 && W <= 8 * sizeof(T), "bitbuffer-width-invalid: W must be in 1..bits of T");
static_assert((T)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
static_assert(!((T)2 == 1) || W == 1, "bitbuffer-width-invalid: a bool element holds one bit");

// NOT `byref` — passed by copy, like `slice` (spec §4.3). `bit` is the offset
// of the first element inside the first byte, in [0, 8).
pub modifier bitslice { size_t len; size_t bit; u8 *words; }

// raw pointer — the program asserts `p` holds `bits.bytes(n, W)` bytes.
pub inline bitslice from(u8 *p, size_t n) {
    return (bitslice){ p ? n : 0, 0, p };
}

pub inline size_t length(bitslice s) { return s.len; }

pub inline T get(bitslice s, size_t i) {
    KEEL_CHECK(i < s.len, "index-out-of-length");
    return (T)bits.read(s.words, s.bit + i * W, W);
}

pub inline void set(bitslice s, size_t i, T v) {
    KEEL_CHECK(i < s.len, "set-out-of-length");
    KEEL_CHECK(W == 64 || ((u64)v >> (W % 64)) == 0, "bit-value-out-of-width");
    bits.write(s.words, s.bit + i * W, W, (u64)v);
}

// the only accessor checked in release (spec §5.8).
pub inline outcome T at(bitslice s, size_t i) {
    outcome T r = {0};
    return i < s.len ? outcome.win(r, get(s, i)) : outcome.none(r);
}

// a slice of a slice: the verb of `x[a..b]` (spec §4.5). Saturates in release.
pub inline bitslice as_slice(bitslice s, size_t a, size_t b) {
    if (b > s.len) b = s.len;
    if (a > b) a = b;
    size_t pos = s.bit + a * W;
    return (bitslice){ b - a, pos & 7, s.words + (pos >> 3) };
}

// allocates in `a` and copies the elements, re-aligned to bit 0 of the new
// storage (spec §5.8); the arena's memory is never const.
pub inline child outcome bitslice clone(parent arena *a, bitslice s) {
    outcome bitslice r = {0};
    u8 *data = arena.alloc(a, u8, bits.bytes(s.len, W));
    if (!data) return outcome.none(r);
    for (size_t i = 0; i < s.len; ++i)
        bits.write(data, i * W, W, bits.read(s.words, s.bit + i * W, W));
    return outcome.win(r, (bitslice){ s.len, 0, data });
}

// `k` disjoint parts covering the view. Part 0 also takes the `lead` elements
// before the first byte boundary, and every other boundary is that boundary
// plus a multiple of the byte group, so two parts never write the same byte
// (spec §5.8, rule 4).
pub inline bitslice partition(bitslice s, size_t k, size_t w) {
    if (k == 0) return (bitslice){ 0, s.bit, s.words };
    size_t g = bits.group(W);
    size_t lead = bits.lead(s.bit, W);
    if (lead > s.len) lead = s.len;
    size_t core = s.len - lead;
    size_t step = core / k + (core % k ? 1 : 0);
    step = (step / g + (step % g ? 1 : 0)) * g;
    size_t lo = w == 0 ? 0 : lead + w * step;
    if (lo >= s.len) return (bitslice){ 0, s.bit, s.words };
    size_t hi = lead + (w + 1) * step;
    if (hi > s.len) hi = s.len;
    return as_slice(s, lo, hi);
}
