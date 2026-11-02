module app3.df04;
import keel.outcome as outcome types;
priv int log_n = 0;
priv void note(int v) { log_n = v; }
pub outcome i32 f(int n) {
    outcome i32 r = {0};
    defer note(n);
    if (n < 0) return outcome.fail(r, 1);
    return outcome.win(r, n);
}
pub int last(void) { return log_n; }
