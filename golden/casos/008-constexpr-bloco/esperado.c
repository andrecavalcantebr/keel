/* gen/app/cx.c — perfil C23: a declaração sai verbatim e os usos não mudam. */
#include "keel/prelude.h"
#include <stdio.h>

typedef struct { int N; int MAX; } app_cx_Cfg;

constexpr size_t app_cx_MAX = 4;

int app_cx_soma(app_cx_Cfg *c) {
    constexpr size_t N = 8;
    i32 v[N];
    int t = 0;
    { for (size_t i = 0; i < N; i++) { v[i] = (i32)i; t += v[i]; } }
    c->N   = (int)N;
    c->MAX = (int)app_cx_MAX;
    return t + (int)sizeof(v) / (int)sizeof(i32);
}

int app_cx_outro(void) {
    { constexpr size_t N = 2; i32 a[N]; a[0] = 1; return (int)(sizeof(a)/sizeof(i32)); }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    app_cx_Cfg c = {0};
    if (app_cx_soma(&c) != 28 + 8) return 1;
    if (c.N != 8 || c.MAX != 4) return 2;
    if (app_cx_outro() != 2) return 3;
    puts("ok");
    return 0;
}
