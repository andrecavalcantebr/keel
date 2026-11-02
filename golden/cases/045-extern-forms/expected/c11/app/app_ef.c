/* app/app_ef.c — generated from app/ef.k by cgen, C11 profile. */
#include "app/app_ef.h"
#line 1 "app/ef.k"










static int cache = 3;




int app_ef_f(T *t) { return twice(*t) + one() + cache + (int)app_ef_K; }
size_t app_ef_g(void) {
#define keel__N_0 ((size_t)(3 + \
        4))
#line 18 "app/ef.k"
    static const size_t keel__N_0__chk = (3 +
        4); (void)keel__N_0__chk;
    char b[keel__N_0];
    return sizeof b;
#undef keel__N_0
#line 22 "app/ef.k"
}
