/* app/app_ef.c — generated from app/ef.k by cgen, C23 profile. */
#include "app/app_ef.h"
#line 1 "app/ef.k"










static int cache = 3;




int app_ef_f(T *t) { return twice(*t) + one() + cache + (int)app_ef_K; }
size_t app_ef_g(void) {
    constexpr size_t N = 3 +
        4;
    char b[N];
    return sizeof b;
}
