/* app/app_mm.c — generated from app/mm.k by cgen, C23 profile. */
#include "app/app_mm.h"
#include "keel/keel_tagged_app_mm_Cycle_void.h"
#line 1 "app/mm.k"





int app_mm_comment_arm(void) {
    keel_tagged_app_mm_Cycle_void s = {0};
    int r = 0;
    switch (s.tag) {
    case app_mm_Cycle_A: goto keel__m0_A;
    case app_mm_Cycle_B: goto keel__m0_B;
    default:             KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 10 "app/mm.k"
    keel__m0_A: { r = 1; }
    goto keel__m0_end;
#line 11 "app/mm.k"
    keel__m0_B: { r = 2; }
    keel__m0_end: ;
    return r;
}

int app_mm_stacked(void) {
    keel_tagged_app_mm_Cycle_void s = {0};
    int r = 0;
    switch (s.tag) {
    case app_mm_Cycle_A: goto keel__m0_A;
    case app_mm_Cycle_B: goto keel__m0_B;
    default:             KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 20 "app/mm.k"
    keel__m0_A: keel__m0_B: {
        r = 1;
    }
    keel__m0_end: ;
#line 23 "app/mm.k"
    return r;
}

int app_mm_oneline(void) {
    keel_tagged_app_mm_Cycle_void s = {0};
    int r = 0;
    switch (s.tag) {
    case app_mm_Cycle_A: goto keel__m0_A;
    case app_mm_Cycle_B: goto keel__m0_B;
    default:             KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
    keel__m0_A: { r = 1; } goto keel__m0_end; keel__m0_B: { r = 2; } keel__m0_end: ;
#line 30 "app/mm.k"
    return r;
}
