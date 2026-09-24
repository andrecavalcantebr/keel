/*
 * DESC: keel slice — fixed-extent view (spec §5.3, backend §5.2, §5.11, §5.13)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.slice type T;

import keel.arena   as arena   types;
import keel.outcome as outcome types;

#include <string.h>

// NOT `byref` — passed by copy (spec §4.3: `slice T` may be passed by
// value). That is what lets `slice.length(slice.of(b))` nest without a
// temporary.
pub modifier slice { size_t len; T *ptr; }

// a declaration that mentions neither `T` nor `slice`: emitted once for the
// module, not per instance (backend §5.11) — it is `keel_slice_cursor`, not
// `keel_slice_T_cursor`.
pub typedef struct { size_t i; } cursor;

pub inline slice from(type T, T *p, size_t n) {
    return (slice){ p ? n : 0, p };
}

pub inline size_t length(slice s) { return s.len; }
pub inline T      get(slice s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return s.ptr[i]; }
pub inline void   set(slice s, size_t i, T v) { KEEL_CHECK(i < s.len, "set-out-of-length"); s.ptr[i] = v; }
pub inline T     *ptr(slice s) { return s.ptr; }
pub inline T     *ptr(slice s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return &s.ptr[i]; }

// the only accessor checked in release (spec §5.3, backend §5.13).
pub inline outcome T at(slice s, size_t i) {
    outcome T r = {0};
    return i < s.len ? outcome.win(r, s.ptr[i]) : outcome.none(r);
}

// a slice of a slice — the arity-2 `of` (spec §4.5: the container's module
// declares its `of`; for a `slice` source, that is this module).
pub inline slice of(slice s, size_t a, size_t b) {
    KEEL_CHECK(a <= b && b <= s.len, "range-index-out-of-bounds");
    if (b > s.len) b = s.len; if (a > b) a = b;
    return (slice){ b - a, s.ptr + a };
}

// allocates in `a`, copies the source's length (spec §5.3).
pub inline outcome slice clone(arena *a, slice s) {
    outcome slice r = {0};
    T *data = arena.alloc(a, T, s.len);
    if (!data) return outcome.none(r);
    if (s.len > 0) memcpy(data, s.ptr, s.len * sizeof(T));
    return outcome.win(r, (slice){ s.len, data });
}

// cursor and partition take the copy, like every `slice` verb.
pub inline cursor begin(slice s) { (void)s; return (cursor){0}; }
pub inline bool   has_next(slice s, cursor *c) { return c->i < s.len; }
pub inline T     *next(slice s, cursor *c) { return &s.ptr[c->i++]; }

// `k` disjoint parts covering the container; the high-index one may come
// out empty (spec §4.8, backend §5.11).
pub inline slice partition(slice s, size_t k, size_t w) {
    if (k == 0) return (slice){0, s.ptr};
    size_t step = s.len / k + (s.len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= s.len) return (slice){0, s.ptr};
    size_t hi = lo + step;
    if (hi > s.len) hi = s.len;
    return (slice){ hi - lo, s.ptr + lo };
}
