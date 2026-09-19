module app.el;
import keel.slice   as slice   types;
import keel.outcome as outcome types;
import_c <stdio.h>;

priv outcome i32 read(int n) {
    outcome i32 r = {0};
    if (n < 0) return outcome.fail(r, 7);
    if (n == 0) return outcome.none(r);
    return outcome.win(r, n * 2);
}
priv outcome slice i32 view(int n, i32 *base) {
    outcome slice i32 r = {0};
    if (n <= 0) return outcome.none(r);
    return outcome.win(r, slice.from(i32, base, (size_t)n));
}

/* EXIT form: the statement goes inside the `if`, verbatim. */
pub int twice(int n) {
    outcome i32 v = read(n) else return -1;
    return outcome.value(v);
}

/* EXIT form with a block. */
pub int twice_log(int n) {
    outcome i32 v = read(n) else { puts("is_fault"); return -1; }
    return outcome.value(v);
}

/* DEFAULT form: `win` takes the result itself and the default value. */
pub size_t size(int n, i32 *base) {
    outcome slice i32 s = view(n, base) else slice.from(i32, base, 0);
    return slice.length(outcome.value(s));   /* verb over verb: it nests */
}
