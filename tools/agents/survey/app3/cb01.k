module app3.cb01;
import keel.outcome as outcome types;
import keel.slice   as slice types;
priv int trail = 0;
priv void mark(int d) { trail = trail * 10 + d; }
priv outcome i32 read(int n) {
    outcome i32 r = {0};
    if (n < 0) return outcome.fail(r, 1);
    return outcome.win(r, n);
}
pub int f(int n) {
    trail = 0;
    for (int i = 0; i < n; i++) {
        defer mark(i);
        outcome i32 v = read(i - 1) else continue;
        mark(outcome.value(v));
    }
    return trail;
}
