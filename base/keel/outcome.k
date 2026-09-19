/*
 * module keel.outcome type T
 *
 * DESC: keel outcome — final result with a code (spec §5.5, backend §5.14)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.outcome type T;

pub modifier outcome { i32 code; T value; }

// mention neither parameter nor modifier: emitted once for the module, not
// per instance (spec §4.3, backend §4.4.1).
pub constexpr i32 OK   = 0;
pub constexpr i32 NONE = INT32_MIN;

// `void` drops the `value` field and every verb that mentions it directly
// (spec §4.3) — win(r,v)/value(r)/value(r,v) vanish on their own in the
// `outcome void` instance, with no special handling here.

pub inline bool failed(outcome e) { return e.code != OK; }
pub inline bool ok    (outcome e) { return e.code == OK; }
pub inline i32  code  (outcome e) { return e.code; }
pub inline T    value (outcome e) { return e.value; }
pub inline void value (outcome *r, T v) { r->value = v; }

// writers take the object, adjust it and return the adjusted copy — leaving
// the function is `return outcome.win(r, value);` (spec §5.5 §4).
pub inline outcome win (outcome *r)         { r->code = OK; return *r; }
pub inline outcome win (outcome *r, T v)    { r->code = OK; r->value = v; return *r; }
pub inline outcome fail(outcome *r, i32 c)  { r->code = c; return *r; }
pub inline outcome none(outcome *r)         { r->code = NONE; return *r; }
