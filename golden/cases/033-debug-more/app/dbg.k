module app.dbg;
import keel.corot   as corot   types;
import keel.outcome as outcome types;
import keel.routine as routine types;
import keel.slice   as slice   types;
import keel.array   as array;
import_c <stdlib.h>;

pub constexpr size_t H = 2;
pub constexpr size_t W = 3;

/* embedded columns under one count per dimension, as in case 020. */
pub extent struct image [h, H] [w, W] {
    array u8 r[H, W];
    size_t h, w;
};

priv corot stage(i16 *counter) {
    corot r = {0};
    ++*counter;
    return corot.win(r);
}

/* One run takes one check by its number (golden DEBUG): each branch breaks a
   rule that only the debug build verifies (backend §5.17), and aborts there. */
pub int main(int argc, char **argv) {
    int which = argc > 1 ? atoi(argv[1]) : 0;
    i16 c = 0;
    if (which == 1) {
        struct image img = {0};
        img.h = 1;
        img.w = 1;
        return img.r[1, 0];
    }
    if (which == 2) {
        array routine.slot i16 steps[2] = {
            { .f = stage, .ctx = &c },
            { .f = stage, .ctx = &c },
        };
        outcome u32 r = routine.par(slice.of(steps), 3);
        return outcome.ok(r) ? 0 : 1;
    }
    if (which == 3) {
        array routine.slot i16 many[65];
        return (int)routine.mask(slice.of(many));
    }
    return 0;
}
