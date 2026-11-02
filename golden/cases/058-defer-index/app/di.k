module app.di;

priv int seen = -1;
pub  int  last(void) { return seen; }

/* The cleanups read `v` at the exit. Each function registers `seen` first, so
   it runs last and records what the other cleanups left in the array. */
pub void clear_var(size_t i) {
    array i32 v[3] = {7, 8, 9};
    defer seen = v[0];
    defer v[i] = 0;
}

/* `return` reads its value before the cleanup: v[0] is 7 here, and the cleanup
   zeroes v[i], which is v[0] only when i is 0. */
pub int take_var(size_t i) {
    array i32 v[3] = {7, 8, 9};
    defer seen = v[0];
    defer v[i] = 0;
    return v[0];
}

/* `[now]` copies `k` at the registration: the `k = 2` after it does not reach
   the cleanup, which zeroes v[1]. */
pub void clear_now(void) {
    array i32 v[3] = {7, 8, 9};
    size_t k = 1;
    defer seen = v[1];
    defer [now size_t k] v[k] = 0;
    k = 2;
}

/* A literal index keeps the plain bracket: the cleanup zeroes v[1] only. */
pub void clear_lit(void) {
    array i32 v[3] = {7, 8, 9};
    defer seen = v[0] * 10 + v[1];
    defer v[1] = 0;
}

/* A braced cleanup with two variable indices; the seen line runs after it. */
pub void two_vars(size_t i, size_t j) {
    array i32 v[3] = {7, 8, 9};
    defer seen = v[0] * 100 + v[1] * 10 + v[2];
    defer {
        v[i] = 1;
        v[j] = 2;
    }
}
