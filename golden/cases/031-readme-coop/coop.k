module coop;
import keel.corot as corot types;
import keel.outcome as outcome types;
import keel.tagged as tagged types;

priv corot count(i16 *counter, i16 limit, bool failure) {
    corot r = {0};
    ++*counter;
    if (*counter < limit) return corot.again(r);
    if (failure)              return corot.fault(r, 1);
    return corot.win(r);
}

priv void note(i16 *counter) {
    ++*counter;
}

pub tags Cycle [WAIT, END];

pub i16 manual(void) {
    tagged Cycle void state = {0};
    tagged.mark(state, WAIT);
    i16 calls = 0;
    bool finished = false;

    while (!finished) {
        match (state) {
            WAIT:
                corot r = count(&calls, 2, false);
                if (corot.faulted(r) || corot.ok(r)) tagged.mark(state, END);
            END:
                finished = true;
        }
    }
    return calls;
}

import keel.routine as routine types;
import keel.slice   as slice   types;
import keel.array   as array;

priv corot stage(i16 *counter) {
    corot r = {0};
    ++*counter;
    if (*counter < 2) return corot.again(r);
    return corot.win(r);
}

pub outcome u32 set_of(i16 *a, i16 *b) {
    array routine.slot i16 steps[2] = {
        { .f = stage, .ctx = a },
        { .f = stage, .ctx = b },
    };

    outcome u32 r = routine.par(slice.of(steps), 1);

    foreach (routine.slot i16 *sl, size_t i : steps) {
        if (corot.ok(routine.state(sl))) note(a);
    }
    return r;
}
