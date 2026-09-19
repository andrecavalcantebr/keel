module app.pos;

import keel.arena as arena types;
import keel.slice as slice types;

/* soa (spec §4.11): `x`/`y` marked `array` become columns; `active`, `len`
   and `cap` pass through as written — no field, verb or allocation on the
   core's account. `pub` because the caller declares the instance and passes
   it by pointer; the `soa` mark repeats on the parameter so that recognizing
   `p[i].field` stays anchored to the declaration, and not to tracking types
   across functions. */
pub soa struct position {
    array f32 x;
    array f32 y;
    bool active;
    size_t len, cap;
};

pub void reserve(soa position *p, arena *a, size_t n) {
    p->x = arena.alloc(a, f32, n);
    p->y = arena.alloc(a, f32, n);
    p->cap = n;
    p->len = 0;
}

pub bool push_row(soa position *p, f32 vx, f32 vy) {
    if (p->len == p->cap) return false;
    p[p->len].x = vx;
    p[p->len].y = vy;
    p->len++;
    return true;
}

/* the bridge: the already existing `slice.from` (§5.3) turns the raw column
   into an ordinary `slice f32`, and `foreach` works with no extra code — the
   same would hold for `walk`, `apply` and `partition`/`parallel`. */
pub f32 sum_x(soa position *p) {
    slice f32 xs = slice.from(f32, p->x, p->len);
    f32 total = 0;
    foreach (f32 v, size_t i : xs) total += v;
    return total;
}
