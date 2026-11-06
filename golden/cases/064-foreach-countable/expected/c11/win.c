/* win.c — generated from win.k by cgen, C11 profile. */
#include "win.h"
#line 1 "win.k"










size_t win_sum_param(win_window *w) {
    size_t total = 0;
    { size_t keel__f0 = win_first(w); size_t keel__l0 = win_limit(w); for (size_t i = keel__f0; i < keel__l0; i++) {
        total += i;
    } }
    return total;
}

size_t win_sum_local(void) {
    win_window w = { 2, 5 };
    size_t total = 0;
    { size_t keel__f0 = win_first(&w); size_t keel__l0 = win_limit(&w); for (size_t i = keel__f0; i < keel__l0; i++) {
        total += i;
    } }
    return total;
}
