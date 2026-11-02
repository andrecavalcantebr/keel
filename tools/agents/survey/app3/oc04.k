module app3.oc04;
import keel.outcome as outcome types;
import keel.slice as slice types;
priv outcome slice i32 view(int n, i32 *base) {
    outcome slice i32 r = {0};
    if (n <= 0) return outcome.none(r);
    return outcome.win(r, slice.from(i32, base, (size_t)n));
}
pub size_t f(int n, i32 *base, size_t fallback) {
    outcome slice i32 s = view(n, base) else
    {
        return fallback;
    }
    return slice.length(outcome.value(s));
}
