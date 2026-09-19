/*
 * DESC: keel corot — cooperative return state (spec §5.5, backend §5.14)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.corot;

// the set names the three regions of the code; `tag` is a reading, not
// storage (spec §5.5) — two different failure codes give the same tag.
pub tags Status [SUCCESS = -1, ONGOING = 0, FAILED = 1];

// a one-field struct, not `typedef i32`, on purpose: an integer typedef
// creates no type in C, and would let `if (r)`, `r == 0` and the mixing with
// an `outcome` code pass in silence (spec §5.5). A struct of one `i32` comes
// back in a register on every current ABI.
pub typedef struct { i32 code; } corot;

pub inline bool ok     (corot r) { return r.code <  0; }
pub inline bool ongoing(corot r) { return r.code == 0; }
pub inline bool faulted(corot r) { return r.code >  0; }
pub inline i32  code   (corot r) { return r.code; }

// reads the tag out of the code — the representation stays a single `i32`,
// and the failure code stays whole (spec §5.5).
pub inline i32  tag    (corot r) {
    return r.code < 0 ? (i32)Status.SUCCESS
         : r.code > 0 ? (i32)Status.FAILED
                      : (i32)Status.ONGOING;
}

// producers are verbs: they adjust the object and return the adjusted copy.
// Leaving the function is `return corot.win(r);`, written in the source.
// `fault` does not normalize the code: zero would mean ongoing and a
// negative would mean success (spec §5.5 §4).
pub inline corot win  (corot *r)         { r->code = -1; return *r; }
pub inline corot again(corot *r)         { r->code =  0; return *r; }
pub inline corot fault(corot *r, i32 c)  { r->code =  c; return *r; }
