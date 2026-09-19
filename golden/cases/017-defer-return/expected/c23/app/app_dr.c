/* app/app_dr.c — generated from app/dr.k by cgen, C23 profile. */
#include "app/app_dr.h"
#line 1 "app/dr.k"





static void app_dr_reset(app_dr_Rec *r) { r->v = 0; }
static void app_dr_mark_a(app_dr_Rec *r) { r->v = r->v * 10 + 1; }
static void app_dr_mark_b(app_dr_Rec *r) { r->v = r->v * 10 + 2; }




int app_dr_consume(app_dr_Rec *r, int mode) {
    int keel__rv0;
    if (mode == 0) { keel__rv0 = r->v; goto keel__e0; }
    keel__rv0 = r->v * 2; goto keel__e0;
keel__e0: app_dr_reset(r); return keel__rv0; }



int app_dr_two(app_dr_Rec *r, int early) {
    int keel__rv0;
    if (early) { keel__rv0 = -1; goto keel__e0; }

    keel__rv0 = -2; goto keel__e1;
keel__e1: app_dr_mark_b(r); keel__e0: app_dr_mark_a(r); return keel__rv0; }


void app_dr_nothing(app_dr_Rec *r) {

    r->v = 9;
    { app_dr_reset(r); return; }
}
