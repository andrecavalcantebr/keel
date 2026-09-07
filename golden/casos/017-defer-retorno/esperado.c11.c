/* gen/app/dr.c — perfil C11.
   Toda saída é `return` ou fim natural → escada de rótulos (backend §5.5.2),
   um degrau por registro, na ordem inversa. O temporário do `return expr` é
   declarado com o tipo de retorno ESCRITO na função (§4.7). */
#include "keel/prelude.h"
#include <stdio.h>

typedef struct { int v; } app_dr_Rec;

static void app_dr_zera(app_dr_Rec *r) { r->v = 0; }
static void app_dr_marca_a(app_dr_Rec *r) { r->v = r->v * 10 + 1; }
static void app_dr_marca_b(app_dr_Rec *r) { r->v = r->v * 10 + 2; }

int app_dr_consome(app_dr_Rec *r, int modo) {
    int keel__rv0;
    if (modo == 0) { keel__rv0 = r->v; goto keel__e0; }
    keel__rv0 = r->v * 2; goto keel__e0;
keel__e0: app_dr_zera(r);
    return keel__rv0;
}

int app_dr_dois(app_dr_Rec *r, int cedo) {
    int keel__rv0;
    if (cedo) { keel__rv0 = -1; goto keel__e0; }   /* só `marca_a` registrado */
    keel__rv0 = -2; goto keel__e1;
keel__e1: app_dr_marca_b(r);                        /* ordem inversa: b antes de a */
keel__e0: app_dr_marca_a(r);
    return keel__rv0;
}

void app_dr_nada(app_dr_Rec *r) {
    r->v = 9;
    { app_dr_zera(r); return; }                     /* void: sem temporário */
}

int main(void) {
    app_dr_Rec r = { 5 };
    if (app_dr_consome(&r, 0) != 5) return 1;   /* valor ANTES do cleanup */
    if (r.v != 0) return 2;                     /* e o cleanup rodou */

    r.v = 5;
    if (app_dr_consome(&r, 1) != 10) return 3;
    if (r.v != 0) return 4;

    r.v = 0;
    if (app_dr_dois(&r, 1) != -1) return 5;
    if (r.v != 1) return 6;                     /* só marca_a: 0*10+1 */

    r.v = 0;
    if (app_dr_dois(&r, 0) != -2) return 7;
    if (r.v != 21) return 8;                    /* b depois a: (0*10+2)*10+1 */

    r.v = 1;
    app_dr_nada(&r);
    if (r.v != 0) return 9;
    puts("ok");
    return 0;
}
