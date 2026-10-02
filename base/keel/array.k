/*
 * DESC: keel array — element access on a bare C array (spec §4.2, §5.3, backend §5.2, §5.18)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.array type T;

import keel.outcome as outcome types;
import keel.slice   as slice   types;
import keel.range   as range   types;

// No type and no modifier: an instance is only the verbs of one element type,
// chosen by the element type of the argument (spec §4.4). `N` is the length
// that arrived (spec §4.2); the constants — length, capacity, dim — stay in
// the core, as `keel.length(v)`.
//
// Each `v[i]` below is an index on an `array`, so the core already checks it
// against `N` with keel_index (backend §5.3, §5.17): no KEEL_CHECK by hand.
pub inline T    get(array T v[size_t N], size_t i)      { return v[i]; }
pub inline void set(array T v[size_t N], size_t i, T x) { v[i] = x; }
pub inline T   *ptr(array T v[size_t N])                { (void)N; return v; }
pub inline T   *ptr(array T v[size_t N], size_t i)      { return &v[i]; }

// the only accessor checked in release (spec §5.3, backend §5.13).
pub inline outcome T at(array T v[size_t N], size_t i) {
    outcome T r = {0};
    return i < N ? outcome.win(r, v[i]) : outcome.none(r);
}

// the range-index protocol (spec §5.1, §5.3): `length` and `as_slice(x, a, b)`.
// The length is the constant `N` that arrived; the verb is this module's, so
// `x[a..b]` over an `array` is `as_slice(x, a, b)` like over any container.
// The backend injects the range check at the top of the three-parameter
// verb (backend §5.17); the body saturates, which is what a release build runs.
pub inline slice as_slice(array T v[size_t N]) { return (slice){ N, v }; }

pub inline slice as_slice(array T v[size_t N], size_t a, size_t b) {
    if (b > N) b = N;
    if (a > b) a = b;
    return (slice){ b - a, v + a };
}

pub inline slice as_slice(array T v[size_t N], range r) { return as_slice(v, r.first, r.limit); }

// the `length` of the protocols (spec §5.1, §5.3): the extent that arrived by
// the binder. `keel.length(v)` stays the core's constant (spec §4.2).
pub inline size_t length(array T v[size_t N]) { (void)v; return N; }
