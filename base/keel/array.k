/*
 * DESC: keel array — element access on a bare C array (spec §4.2, §5.3, backend §5.2, §5.18)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.array type T;

import keel.outcome as outcome types;

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
