module app3.mt05;
import keel.tagged as tagged types;
pub tags Cycle [A, B];
pub int f(tagged Cycle void *s) {
    int r = 0;
    match (s) {
        A:
            /* arm with a comment */
            if (r == 0) r = 5;
            else r = 6;
            break;
        B:
            r = 7;
    }
    return r;
}
