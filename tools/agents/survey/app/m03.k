module app.m03;
import keel.tagged as tagged types;
pub tags Cycle [A, B];
pub int f(int n) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) {
        A:
            for (int i = 0; i < n; i++) r += i;
            break;
        B:
            r = 2;
    }
    return r;
}
