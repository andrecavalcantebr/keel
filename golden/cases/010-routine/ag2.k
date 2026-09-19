module ag2;
import keel.corot   as corot   types;
import keel.outcome as outcome types;
import keel.routine as routine types;
import keel.slice   as slice   types;

pub typedef struct { i32 a, b, c; } Ag;

priv corot hello(Ag *g)       { corot r = {0}; if (g->a-- > 0) return corot.again(r); return corot.win(r); }
priv corot authenticate(Ag *g) { corot r = {0}; if (g->b-- > 0) return corot.again(r); return corot.win(r); }
priv corot ready(Ag *g)    { corot r = {0}; if (g->c-- > 0) return corot.again(r); return corot.win(r); }
priv corot faults(Ag *g)    { corot r = {0}; return corot.fault(r, 9); }

/* Composition is a function call: what used to sit after an exit label is now
   the statement following the result's declaration (§5.6). */
pub outcome u32 chain(Ag *g) {
    array routine.slot Ag steps[3] = {
        { .f = hello,       .ctx = g },
        { .f = authenticate, .ctx = g },
        { .f = ready,    .ctx = g },
    };
    return routine.seq(slice.of(steps));
}

/* FAILED ends the sequence: the third stage is never called, and its state
   stays ONGOING in the table. */
pub outcome u32 broken_chain(Ag *g, i32 *third_state) {
    array routine.slot Ag steps[3] = {
        { .f = hello,    .ctx = g },
        { .f = faults, .ctx = g },
        { .f = ready, .ctx = g },
    };
    outcome u32 r = routine.seq(slice.of(steps));
    *third_state = routine.code(steps[2]);
    return r;
}

/* Target one of two: the policy is evaluated at the end of the cycle, so both
   are called in the round where the first one wins. */
pub outcome u32 together(Ag *g) {
    array routine.slot Ag steps[2] = {
        { .f = hello,       .ctx = g },
        { .f = authenticate, .ctx = g },
    };
    return routine.par(slice.of(steps), 1);
}

/* Target two of three, with one failure: still possible. */
pub outcome u32 two(Ag *g) {
    array routine.slot Ag steps[3] = {
        { .f = faults,    .ctx = g },
        { .f = hello,       .ctx = g },
        { .f = authenticate, .ctx = g },
    };
    return routine.par(slice.of(steps), 2);
}

/* Two failures out of three make a target of two impossible. */
pub outcome u32 impossible(Ag *g) {
    array routine.slot Ag steps[3] = {
        { .f = faults, .ctx = g },
        { .f = faults, .ctx = g },
        { .f = ready, .ctx = g },
    };
    return routine.par(slice.of(steps), 2);
}
