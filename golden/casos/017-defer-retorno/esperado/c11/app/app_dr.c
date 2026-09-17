/* gen/app/dr.c — gerado de app/dr.k, perfil C11.
   Toda saída é `return` ou fim natural → escada de rótulos (backend §5.5.2),
   um degrau por registro, na ordem inversa. O temporário do `return expr` é
   declarado com o tipo de retorno ESCRITO na função (§4.7). */

#include "keel/keel.type.h"
#include "app/app_dr.impl.h"
#line 6 "app/dr.k"
static void app_dr_zera(app_dr_Rec *r) { r->v = 0; }
static void app_dr_marca_a(app_dr_Rec *r) { r->v = r->v * 10 + 1; }
static void app_dr_marca_b(app_dr_Rec *r) { r->v = r->v * 10 + 2; }

#line 13 "app/dr.k"
int app_dr_consome(app_dr_Rec *r, int modo) {
    int keel__rv0;
    if (modo == 0) { keel__rv0 = r->v; goto keel__e0; }
    keel__rv0 = r->v * 2; goto keel__e0;
keel__e0: app_dr_zera(r);
    return keel__rv0;
}

#line 21 "app/dr.k"
int app_dr_dois(app_dr_Rec *r, int cedo) {
    int keel__rv0;
    if (cedo) { keel__rv0 = -1; goto keel__e0; }   /* só `marca_a` registrado */
    keel__rv0 = -2; goto keel__e1;
keel__e1: app_dr_marca_b(r);                        /* ordem inversa: b antes de a */
keel__e0: app_dr_marca_a(r);
    return keel__rv0;
}

#line 28 "app/dr.k"
void app_dr_nada(app_dr_Rec *r) {
    r->v = 9;
    { app_dr_zera(r); return; }                     /* void: sem temporário */
}
