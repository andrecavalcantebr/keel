module app.t06;
import keel.tagged as tagged types;
pub tags Cycle [A, B];
pub int f(int n) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) {
        A:
            switch (n) { case 1: r = 5; break; default: break; }
            break;
        B:
            r = 2;
    }
    return r;
}
