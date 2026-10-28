/* cb.c — generated from cb.k by cgen, C23 profile. */
#include "cb.h"
#line 1 "cb.k"


size_t cb_twice(void) {
    constexpr size_t N = 8;
    int buf[N];
    return sizeof(buf) / sizeof(int) + N;
}

size_t cb_other(void) {
    { constexpr size_t N = 2; return N; }
}

size_t cb_third(void) {
    constexpr size_t M = 2 + 3;
    return M;
}


