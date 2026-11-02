module app3.oc03;
import keel.array as array;
import keel.outcome as outcome types;
import keel.routine as routine types;
import keel.slice   as slice types;
import keel.corot   as corot types;
priv corot hop(i32 *n) { corot r = {0}; if (*n-- > 0) return corot.again(r); return corot.win(r); }
pub int two(i32 a, i32 b) {
    i32 x = a, y = b;
    array routine.slot i32 steps[2] = { { .f = hop, .ctx = &x }, { .f = hop, .ctx = &y } };
    outcome u32 r = routine.par(slice.of(steps), 1);
    return outcome.value(r);
}
