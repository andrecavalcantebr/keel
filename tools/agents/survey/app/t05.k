module app.t05;
import keel.tagged as tagged types;
pub tags Cycle [A, B];
pub int f(void) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) { A: r = 1; B: r = 2; }
    return r;
}
