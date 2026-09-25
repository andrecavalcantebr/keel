/* app/app_cx.c — generated from app/cx.k by cgen, C23 profile. */
#include "app/app_cx.h"
#line 1 "app/cx.k"










int app_cx_sum(app_cx_Cfg *c) {
    constexpr size_t N = 8;
    i32 v[N];
    int t = 0;
    { for (size_t i = 0; i < N; i++) { v[keel_index(i, N)] = (i32)i; t += v[keel_index(i, N)]; } }
    c->N   = (int)N;
    c->MAX = (int)app_cx_MAX;
    return t + (int)sizeof(v) / (int)sizeof(i32);
}


int app_cx_other(void) {
    { constexpr size_t N = 2; i32 a[N]; a[0] = 1; return (int)(sizeof(a)/sizeof(i32)); }
}

int app_cx_main(int argc, char **argv) {
    app_cx_Cfg c = {0};
    if (app_cx_sum(&c) != 28 + 8) return 1;
    if (c.N != 8 || c.MAX != 4) return 2;
    if (app_cx_other() != 2) return 3;
    puts("ok");
    return 0;
}
