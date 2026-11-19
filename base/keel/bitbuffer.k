/*
 * DESC: keel bitbuffer — variable-length sequence of W-bit elements (spec §5.8, backend §5.20)
 * AUTHOR: andre cavalcante
 * DATE: Nov, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.bitbuffer dim W type T;

import keel.bitslice as bitslice;
import keel.bits     as bits;
import keel.outcome  as outcome types;
import keel.range    as range   types;
import keel.arena    as arena   types;

// the checks of `bitslice` (spec §5.8, rule 1), once more: each module is its
// own expansion. They run wherever the instance is used, because the verb is
// emitted with it. A file-level `static_assert` that names `W` has no home in
// a generic module, hence the body (the device of keel_coll's `bstree`).
pub inline void assert_instance(void) {
    static_assert(W >= 1 && W <= 8 * sizeof(T), "bitbuffer-width-invalid: W must be in 1..bits of T");
    static_assert((T)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
    static_assert(!((T)2 == 1) || W == 1, "bitbuffer-width-invalid: a bool element holds one bit");
}

// `byref`, like `buffer`: `cap` and `len` count elements, not bits or bytes.
pub modifier bitbuffer byref { size_t cap, len; u8 *words; }

// allocates the bytes for `cap` elements in the arena; no room gives capacity
// zero, and the insertions fail through the normal channel (spec §5.8).
pub inline child bitbuffer make(parent arena *a, size_t cap) {
    bitbuffer r = {0};
    u8 *data = arena.alloc(a, u8, bits.bytes(cap, W));
    if (data) { r.cap = cap; r.words = data; }
    return r;
}

// raw pointer — starts empty; the program asserts `p` holds `bits.bytes(cap, W)` bytes.
pub inline bitbuffer from(u8 *p, size_t cap) {
    return (bitbuffer){ p ? cap : 0, 0, p };
}

// starts full, `len == cap`.
pub inline bitbuffer of(u8 *p, size_t n) {
    return (bitbuffer){ p ? n : 0, p ? n : 0, p };
}

pub inline size_t length(bitbuffer *b) { return b->len; }
pub inline size_t capacity(bitbuffer *b) { return b->cap; }
pub inline size_t bytes(bitbuffer *b) { return bits.bytes(b->cap, W); }

pub inline T get(bitbuffer *b, size_t i) {
    KEEL_CHECK(i < b->len, "index-out-of-length");
    return (T)bits.read(b->words, i * W, W);
}

pub inline void set(bitbuffer *b, size_t i, T v) {
    KEEL_CHECK(i < b->len, "set-out-of-length");
    KEEL_CHECK(W == 64 || ((u64)v >> (W % 64)) == 0, "bit-value-out-of-width");
    bits.write(b->words, i * W, W, (u64)v);
}

pub inline bool push(bitbuffer *b, T v) {
    if (b->len == b->cap) return false;
    KEEL_CHECK(W == 64 || ((u64)v >> (W % 64)) == 0, "bit-value-out-of-width");
    bits.write(b->words, b->len * W, W, (u64)v);
    b->len++;
    return true;
}

pub inline outcome T pop(bitbuffer *b) {
    outcome T r = {0};
    if (b->len == 0) return outcome.none(r);
    b->len--;
    return outcome.win(r, (T)bits.read(b->words, b->len * W, W));
}

pub inline void clear(bitbuffer *b) { b->len = 0; }

// the only accessor checked in release (spec §5.8).
pub inline outcome T at(bitbuffer *b, size_t i) {
    outcome T r = {0};
    return i < b->len ? outcome.win(r, (T)bits.read(b->words, i * W, W)) : outcome.none(r);
}

// slicing is a memory verb, not a view verb (rationale, "Memória e visão").
pub inline child bitslice.bitslice(W) T as_slice(parent bitbuffer *b) {
    return (bitslice.bitslice(W) T){ b->len, 0, b->words };
}

pub inline child bitslice.bitslice(W) T as_slice(parent bitbuffer *b, size_t a, size_t c) {
    if (c > b->len) c = b->len;
    if (a > c) a = c;
    size_t pos = a * W;
    return (bitslice.bitslice(W) T){ c - a, pos & 7, b->words + (pos >> 3) };
}

pub inline child bitslice.bitslice(W) T as_slice(parent bitbuffer *b, range r) {
    return as_slice(b, r.first, r.limit);
}

// allocates in `a`, copies the source's length (spec §5.8); the arena's memory is never const.
pub inline child outcome bitbuffer clone(parent arena *a, bitbuffer *b) {
    outcome bitbuffer r = {0};
    u8 *data = arena.alloc(a, u8, bits.bytes(b->cap, W));
    if (!data) return outcome.none(r);
    size_t n = bits.bytes(b->len, W);
    for (size_t i = 0; i < n; ++i) data[i] = b->words[i];
    return outcome.win(r, (bitbuffer){ b->cap, b->len, data });
}

// `k` disjoint parts covering the container; see `bitslice.partition`.
pub inline child bitslice.bitslice(W) T partition(parent bitbuffer *b, size_t k, size_t w) {
    return bitslice.partition(as_slice(b), k, w);
}
