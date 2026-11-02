/* app/app_fm.c — generated from app/fm.k by cgen, C23 profile. */
#include "app/app_fm.h"
#include "app/lib/app_lib_other.h"
#line 1 "app/fm.k"



int app_fm_foreign(app_lib_other_Gate *g) {
    int r = 0;
    switch (g->ph.tag) {
    case app_lib_other_Phase_ON:  goto keel__m0_ON;
    case app_lib_other_Phase_OFF: goto keel__m0_OFF;
    default:                      KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 7 "app/fm.k"
    keel__m0_ON: {
        r = 1;
    }
    goto keel__m0_end;
#line 9 "app/fm.k"
    keel__m0_OFF: {
        r = 2;
    }
    keel__m0_end: ;
#line 12 "app/fm.k"
    return r;
}
