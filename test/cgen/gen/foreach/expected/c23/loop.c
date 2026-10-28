/* loop.c — generated from loop.k by cgen, C23 profile. */
#include "loop.h"
#line 1 "loop.k"


size_t loop_sum(size_t n) {
    size_t t = 0;
    { for (size_t i = 0; i < n; i++) { t += i; } }
    { for (size_t j = 2; j < n; j++) { t += j; } }
    { for (u32 k = 1; k < 4; k++) { t += k; } }
    return t;
}
