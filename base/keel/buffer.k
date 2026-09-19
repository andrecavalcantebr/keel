/*
 * DESC: keel buffer — variable-length sequence (spec §5.3, backend §5.2, §5.11, §5.13)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.buffer type T;

import keel.slice   as slice   types;
import keel.outcome as outcome types;
import keel.arena   as arena   types;

#include <string.h>

// `byref` rejects by-value parameters and diagnoses copies between
// instances (spec §4.3) — the opposite of `slice`.
pub modifier buffer byref { size_t cap, len; T *ptr; }

// a declaration that mentions neither `T` nor `buffer`: emitted once for the
// module, not per instance (backend §5.11) — `keel_buffer_cursor`.
pub typedef struct { size_t i; } cursor;

// raw pointer — starts empty (spec §5.3 §4).
pub inline buffer from(T *p, size_t n) {
    return (buffer){ p ? n : 0, 0, p };
}

// `buffer.of(v)`: over an `array` — starts full, `len == cap`.
pub inline buffer of(T *p, size_t n) {
    return (buffer){ p ? n : 0, p ? n : 0, p };
}

pub inline size_t length(buffer *b) { return b->len; }
pub inline size_t capacity(buffer *b) { return b->cap; }
pub inline T      get(buffer *b, size_t i) { return b->ptr[i]; }
pub inline void   set(buffer *b, size_t i, T v) { b->ptr[i] = v; }
pub inline T     *ptr(buffer *b) { return b->ptr; }
pub inline T     *ptr(buffer *b, size_t i) { return &b->ptr[i]; }

pub inline T *push(buffer *b) {
    if (b->len == b->cap) return NULL;
    return &b->ptr[b->len++];
}

pub inline T *push(buffer *b, T v) {
    if (b->len == b->cap) return NULL;
    b->ptr[b->len] = v;
    return &b->ptr[b->len++];
}

pub inline T *pop(buffer *b) {
    if (b->len == 0) return NULL;
    return &b->ptr[--b->len];
}

pub inline void clear(buffer *b) { b->len = 0; }

// the only accessor checked in release (spec §5.3, backend §5.13).
pub inline outcome T at(buffer *b, size_t i) {
    outcome T r = {0};
    return i < b->len ? outcome.win(r, b->ptr[i]) : outcome.none(r);
}

// slicing is a buffer verb, not a slice verb: the memory side knows the
// view side, never the reverse (rationale, "Memória e visão"), which is
// what keeps the import graph acyclic.
pub inline slice as_slice(buffer *b) {
    return (slice){ b->len, b->ptr };
}

pub inline slice as_slice(buffer *b, size_t a, size_t c) {
    if (a > c || c > b->len) return (slice){0, b->ptr};
    return (slice){ c - a, b->ptr + a };
}

// allocates in `a`, copies the source's length (spec §5.3).
pub inline outcome buffer clone(arena *a, buffer *b) {
    outcome buffer r = {0};
    T *data = arena.alloc(a, T, b->cap);
    if (!data) return outcome.none(r);
    if (b->len > 0) memcpy(data, b->ptr, b->len * sizeof(T));
    return outcome.win(r, (buffer){ b->cap, b->len, data });
}

// cursor: `walk` asks for `begin`, `has_next` and `next`; `next` returns the
// element's address, so the binder is by pointer (spec §5.3).
pub inline cursor begin(buffer *b) { (void)b; return (cursor){0}; }
pub inline bool   has_next(buffer *b, cursor *c) { return c->i < b->len; }
pub inline T     *next(buffer *b, cursor *c) { return &b->ptr[c->i++]; }

// `k` disjoint parts covering the container; the high-index one may come
// out empty. The product is `slice T` (spec §4.8, backend §5.11).
pub inline slice partition(buffer *b, size_t k, size_t w) {
    if (k == 0) return (slice){0, b->ptr};
    size_t step = b->len / k + (b->len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= b->len) return (slice){0, b->ptr};
    size_t hi = lo + step;
    if (hi > b->len) hi = b->len;
    return (slice){ hi - lo, b->ptr + lo };
}
