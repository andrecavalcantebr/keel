/* The harness of case 032 — NOT transpiler output.
 *
 * Each call checks the return value and the order of the cleanups the exit ran
 * (spec §4.6, rules 1 and 3: inner scopes are cleaned before outer ones, and an
 * exit before a registration does not run it). */
#include <assert.h>
#include <string.h>
#include "app/app_dj.h"

static void trail_is(const char *want) {
    size_t n = strlen(want);
    assert(app_dj_walked_now() == n);
    for (size_t i = 0; i < n; i++) assert(app_dj_step(i) == want[i]);
}

int main(void) {
    /* `goto` leaves the block: its cleanup runs, and the label is past it */
    app_dj_reset();
    assert(app_dj_jumped(0) == -1);
    trail_is("g");

    /* no jump: the body runs, and the block's natural end cleans up */
    app_dj_reset();
    assert(app_dj_jumped(1) == 0);
    trail_is("bg");

    /* a `return` from the inner block: the inner cleanup registered before it,
       then the function's ladder. The third `defer` is registered after, and
       does not run. */
    app_dj_reset();
    assert(app_dj_nested(0) == 1);
    trail_is("21");

    app_dj_reset();
    assert(app_dj_nested(1) == 2);
    trail_is("21");

    /* the block ends naturally: both of its cleanups run, in reverse order of
       registration, and the function's ladder follows */
    app_dj_reset();
    assert(app_dj_nested(2) == 3);
    trail_is("321");
    return 0;
}
