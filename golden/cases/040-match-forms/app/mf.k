module app.mf;
import keel.tagged as tagged types;

pub tags Cycle [A, B];
pub typedef struct { tagged Cycle void state; } Holder;

pub int member(Holder *h) {
    int r = 0;
    match (h->state) {
        A:
            r = 1;
        B:
            r = 2;
    }
    return r;
}

pub int inline_arms(void) {
    tagged Cycle void s = {0};
    int r = 0;
    tagged.mark(s, B);
    match (s) {
        A: r = 1;
        B: r = 2;
    }
    return r;
}

pub int with_loop(int n) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) {
        A:
            for (int i = 0; i < n; i++) {
                if (i == 3) break;
                r += i;
            }
            break;
        B:
            r = 100;
    }
    return r + 1;
}
