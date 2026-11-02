module app.t04;
import keel.tagged as tagged types;
pub tags Cycle [A, B];
pub int f(void) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) {
        A: B:
            r = 1;
    }
    return r;
}
