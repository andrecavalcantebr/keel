module app.l02;
import keel.outcome as outcome types;
priv outcome i32 read(int n) {
    outcome i32 r = {0};
    if (n < 0) return outcome.fail(r, 7);
    return outcome.win(r, n);
}
pub int f(int n) {
    outcome i32 v =
        read(n) else return -1;
    return outcome.value(v);
}
