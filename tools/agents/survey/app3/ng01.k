module app3.ng01;
import keel.array as array;
import keel.routine as routine types;
import keel.slice   as slice types;
import keel.corot   as corot types;
priv corot hop(int *n) { corot r = {0}; if (*n-- > 0) return corot.again(r); return corot.win(r); }
pub int two(int a, int b) {
    int x = a, y = b;
    array routine.slot int steps[2] = { { .f = hop, .ctx = &x }, { .f = hop, .ctx = &y } };
    return 0;
}
