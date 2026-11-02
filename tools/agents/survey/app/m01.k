module app.m01;
import keel.tagged as tagged types;
pub tags Cycle [A, B];
pub struct Holder { tagged Cycle void state; };
pub int f(struct Holder *h) {
    int r = 0;
    match (h->state) {
        A:
            r = 1;
        B:
            r = 2;
    }
    return r;
}
