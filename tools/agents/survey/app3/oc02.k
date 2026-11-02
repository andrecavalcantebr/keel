module app3.oc02;
import keel.outcome as outcome types;
import keel.slice   as slice types;
pub outcome void check(int n) {
    outcome void r = {0};
    if (n < 0) return outcome.fail(r, n);
    return outcome.win(r);
}
pub int g(int n) {
    outcome void r = check(n);
    defer n = 0;
    return outcome.code(r);
}
