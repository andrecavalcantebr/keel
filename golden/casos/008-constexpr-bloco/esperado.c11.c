/* gen/app/cx.c — perfil C11 (backend §9.2).
   O de arquivo já é único pelo prefixo do módulo; o de bloco recebe nome
   gerado e os usos são reescritos — `c->N` fica intacto porque IDENT depois
   de `.` ou `->` não é reescrito. */
#include "keel/prelude.h"
#include <stdio.h>

typedef struct { int N; int MAX; } app_cx_Cfg;

#define app_cx_MAX ((size_t)4)
static const size_t app_cx_MAX__chk = 4;

int app_cx_soma(app_cx_Cfg *c) {
#define keel__N_0 ((size_t)8)
    static const size_t keel__N_0__chk = 8;
    i32 v[keel__N_0];
    int t = 0;
    { for (size_t i = 0; i < keel__N_0; i++) { v[i] = (i32)i; t += v[i]; } }
    c->N   = (int)keel__N_0;
    c->MAX = (int)app_cx_MAX;
    (void)keel__N_0__chk;
    return t + (int)sizeof(v) / (int)sizeof(i32);
#undef keel__N_0
}

int app_cx_outro(void) {
    {
#define keel__N_1 ((size_t)2)
    static const size_t keel__N_1__chk = 2;
    i32 a[keel__N_1]; a[0] = 1; (void)keel__N_1__chk;
    return (int)(sizeof(a)/sizeof(i32));
#undef keel__N_1
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    (void)app_cx_MAX__chk;
    app_cx_Cfg c = {0};
    if (app_cx_soma(&c) != 28 + 8) return 1;
    if (c.N != 8 || c.MAX != 4) return 2;
    if (app_cx_outro() != 2) return 3;
    puts("ok");
    return 0;
}
