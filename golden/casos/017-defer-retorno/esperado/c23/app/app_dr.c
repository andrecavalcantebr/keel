/* app/app_dr.c — gerado de app/dr.k pelo cgen, perfil C23. */
#include "app/app_dr.h"
#line 1 "app/dr.k"





static void app_dr_zera(app_dr_Rec *r) { r->v = 0; }
static void app_dr_marca_a(app_dr_Rec *r) { r->v = r->v * 10 + 1; }
static void app_dr_marca_b(app_dr_Rec *r) { r->v = r->v * 10 + 2; }




int app_dr_consome(app_dr_Rec *r, int modo) {
    int keel__rv0;
    if (modo == 0) { keel__rv0 = r->v; goto keel__e0; }
    keel__rv0 = r->v * 2; goto keel__e0;
keel__e0: app_dr_zera(r); return keel__rv0; }



int app_dr_dois(app_dr_Rec *r, int cedo) {
    int keel__rv0;
    if (cedo) { keel__rv0 = -1; goto keel__e0; }

    keel__rv0 = -2; goto keel__e1;
keel__e1: app_dr_marca_b(r); keel__e0: app_dr_marca_a(r); return keel__rv0; }


void app_dr_nada(app_dr_Rec *r) {

    r->v = 9;
    { app_dr_zera(r); return; }
}
