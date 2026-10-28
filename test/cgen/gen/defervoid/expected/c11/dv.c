/* dv.c — generated from dv.k by cgen, C11 profile. */
#include "dv.h"
#line 1 "dv.k"




static void dv_reset(dv_Rec *r) { r->v = 0; }
static void dv_mark_a(dv_Rec *r) { r->v = r->v * 10 + 1; }
static void dv_mark_b(dv_Rec *r) { r->v = r->v * 10 + 2; }



void dv_early(dv_Rec *r, int stop) {

    if (stop) { goto keel__e0; }

    r->v = r->v + 100;
#line 14 "dv.k"
keel__e1: dv_mark_b(r);
#line 12 "dv.k"
keel__e0: dv_mark_a(r);
#line 16 "dv.k"
}



void dv_guard(dv_Rec *r, int stop) {

    if (stop) { goto keel__e0; }
    r->v = 9;
#line 21 "dv.k"
keel__e0: dv_reset(r);
#line 24 "dv.k"
}


void dv_before(dv_Rec *r, int stop) {
    if (stop) return;

    r->v = 5;
#line 29 "dv.k"
keel__e0: dv_reset(r);
#line 31 "dv.k"
}
