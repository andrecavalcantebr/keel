module app.mm;
import keel.tagged as tagged types;

pub tags Cycle [A, B];

pub int comment_arm(void) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) {
        A: r = 1;   // one
        B: r = 2;
    }
    return r;
}

pub int stacked(void) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) {
        A: B:
            r = 1;
    }
    return r;
}

pub int oneline(void) {
    tagged Cycle void s = {0};
    int r = 0;
    match (s) { A: r = 1; B: r = 2; }
    return r;
}
