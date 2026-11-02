module app3.el04;
import keel.outcome as outcome types;
priv outcome i32 read(int n) {
    outcome i32 r = {0};
    if (n < 0) return outcome.fail(r, 7);
    return outcome.win(r, n);
}
pub int sum(int n) {
    int t = 0;
    for (int i = 0; i < n; i++) {
        outcome i32 v = read(i - 1) else continue;
        t += outcome.value(v);
    }
    return t;
}
