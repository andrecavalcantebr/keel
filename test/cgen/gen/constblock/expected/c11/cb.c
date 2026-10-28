/* cb.c — generated from cb.k by cgen, C11 profile. */
#include "cb.h"
#line 1 "cb.k"


size_t cb_twice(void) {
#define keel__N_0 ((size_t)8)
#line 4 "cb.k"
    static const size_t keel__N_0__chk = 8; (void)keel__N_0__chk;
    int buf[keel__N_0];
    return sizeof(buf) / sizeof(int) + keel__N_0;
#undef keel__N_0
#line 7 "cb.k"
}

size_t cb_other(void) {
#define keel__N_1 ((size_t)2)
#line 10 "cb.k"
    { static const size_t keel__N_1__chk = 2; (void)keel__N_1__chk; return keel__N_1; }
#undef keel__N_1
#line 11 "cb.k"
}
