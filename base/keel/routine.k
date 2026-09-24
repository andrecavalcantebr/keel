/*
 * DESC: keel routine — cooperative composition over a table (spec §5.6, backend §5.10)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.routine type C;

import keel.corot   as corot   types;
import keel.outcome as outcome types;
import keel.slice   as slice   types;

// the participant's signature, and the only one the table takes. What a
// participant produces travels through the context, not through the return
// (spec §5.6). It is an instance typedef: the program writes function names
// in the `.f` fields and never has to spell it.
pub typedef corot (*routine)(C *ctx);

// the table's line is a `slot`, and not an `entry`, because it is not only
// the entry: it holds the function, the context and the state that
// participant finished with. Taking the state out would need a second
// sequence parallel to the first, with two lengths to keep in agreement
// (spec §5.6).
pub modifier slot { routine f; C *ctx; corot state; }

pub inline corot state(const slot *s) { return s->state; }
pub inline i32   code (const slot *s) { return s->state.code; }

// the slots are steps in the order of the slice. The current step is called
// until it returns something other than ONGOING; a failure ends the sequence
// without calling the following ones (spec §5.6).
pub inline outcome u32 seq(slice slot s) {
    outcome u32 r = {0};
    size_t n = slice.length(s);
    u32 S = 0;
    for (size_t i = 0; i < n; ++i) slice.ptr(s, i)->state = (corot){0};
    for (size_t i = 0; i < n; ++i) {
        slot *p = slice.ptr(s, i);
        corot c;
        do { c = p->f(p->ctx); } while (corot.ongoing(c));
        p->state = c;
        if (corot.faulted(c)) { outcome.value(r, S); return outcome.fail(r, 1); }
        ++S;
    }
    return outcome.win(r, S);
}

// cycles: each one calls every slot still ONGOING, once, in the order of the
// slice. The policy is judged at the end of the cycle, so more slots may win
// in a cycle than the minimum asked for (spec §5.6).
pub inline outcome u32 par(slice slot s, u32 target) {
    outcome u32 r = {0};
    size_t n = slice.length(s);
    KEEL_CHECK(target <= n, "par-target-above-total");
    u32 m = (u32)n, S = 0, F = 0, q = target ? target : m;
    for (size_t i = 0; i < n; ++i) slice.ptr(s, i)->state = (corot){0};
    for (;;) {
        for (size_t i = 0; i < n; ++i) {
            slot *p = slice.ptr(s, i);
            if (!corot.ongoing(p->state)) continue;
            p->state = p->f(p->ctx);
            if (corot.ok(p->state))           ++S;
            else if (corot.faulted(p->state)) ++F;
        }
        if (S >= q)    return outcome.win(r, S);
        if (m - F < q) { outcome.value(r, S); return outcome.fail(r, 1); }
    }
}

// a convenience derived from the table, not the representation of the result
// (spec §5.6). Above 64 slots the extra ones have no bit: `mask-above-64-slots`.
pub inline u64 mask(slice slot s) {
    u64 m = 0;
    size_t n = slice.length(s);
    KEEL_CHECK(n <= 64, "mask-above-64-slots");
    for (size_t i = 0; i < n && i < 64; ++i)
        if (corot.ok(slice.ptr(s, i)->state)) m |= (u64)1 << i;
    return m;
}
