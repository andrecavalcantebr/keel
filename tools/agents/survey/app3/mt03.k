module app3.mt03;
import keel.corot as corot types;
priv corot tick(int *n) {
    corot r = {0};
    if (*n > 0) { (*n)--; return corot.again(r); }
    return corot.win(r);
}
pub int f(int n) {
    int left = n;
    int out = 0;
    corot st = {0};
    match (st) {
        SUCCESS:
            out = 1;
            break;
        ONGOING:
            st = tick(&left);
            out = 2;
            break;
        FAILED:
            out = corot.code(st);
    }
    return out;
}
