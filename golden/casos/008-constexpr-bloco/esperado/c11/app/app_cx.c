/* app/app_cx.c — gerado de app/cx.k pelo cgen, perfil C11. */
#include "app/app_cx.h"
#line 1 "app/cx.k"









int app_cx_soma(app_cx_Cfg *c) {
#define keel__N_0 ((size_t)8)
    static const size_t keel__N_0__chk = 8; (void)keel__N_0__chk;
#line 12 "app/cx.k"
    i32 v[keel__N_0];
    int t = 0;
    { for (size_t i = 0; i < keel__N_0; i++) { v[i] = (i32)i; t += v[i]; } }
    c->N   = (int)keel__N_0;
    c->MAX = (int)app_cx_MAX;
    return t + (int)sizeof(v) / (int)sizeof(i32);
#undef keel__N_0
}

int app_cx_outro(void) {
    {
#define keel__N_1 ((size_t)2)
    static const size_t keel__N_1__chk = 2; (void)keel__N_1__chk;
    i32 a[keel__N_1]; a[0] = 1; return (int)(sizeof(a)/sizeof(i32));
#undef keel__N_1
    }
}
#line 25 "app/cx.k"
int app_cx_main(int argc, char **argv) {
    app_cx_Cfg c = {0};
    if (app_cx_soma(&c) != 28 + 8) return 1;
    if (c.N != 8 || c.MAX != 4) return 2;
    if (app_cx_outro() != 2) return 3;
    puts("ok");
    return 0;
}
