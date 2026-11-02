module app.t02;
import keel.tagged as tagged types;
pub tags Cycle [A, B];
pub int f(void) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) {
        A: r = 1;   // one
        B: r = 2;
    }
    return r;
}
