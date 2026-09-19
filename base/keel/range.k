/*
 * DESC: keel range — half-open interval (spec §5.3, backend §5.11)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.range;

// `range` is a type, not a modifier: it has no element type. `a..b` in the
// source builds one through `of` (spec §5.3).
pub typedef struct { size_t first, limit; } range;

// an inverted interval collapses to the empty one: subtraction in `size_t`
// would not correct it, and `length` has to stay a subtraction.
pub inline range of(size_t a, size_t b) { return (range){ a, b < a ? a : b }; }

pub inline size_t first (range r) { return r.first; }
pub inline size_t limit (range r) { return r.limit; }
pub inline size_t length(range r) { return r.limit - r.first; }
pub inline size_t get   (range r, size_t i) { return r.first + i; }

// no `ptr`: a range supplies no element storage (spec §5.3 §4). No cursor
// either — `walk` needs an address per element, and there is none.

// `k` disjoint parts covering the interval; the product is a `range`, not a
// view (spec §4.8, backend §5.11).
pub inline range partition(range r, size_t k, size_t w) {
    if (k == 0) return (range){ r.first, r.first };
    size_t n = r.limit - r.first;
    size_t step = n / k + (n % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= n) return (range){ r.limit, r.limit };
    size_t hi = lo + step;
    if (hi > n) hi = n;
    return (range){ r.first + lo, r.first + hi };
}
