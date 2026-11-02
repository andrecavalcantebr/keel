/* app/app_mf.c — generated from app/mf.k by cgen, C11 profile. */
#include "app/app_mf.h"
#include "keel/keel_tagged_app_mf_Cycle_void.h"
#line 1 "app/mf.k"






int app_mf_member(app_mf_Holder *h) {
    int r = 0;
    switch (h->state.tag) {
    case app_mf_Cycle_A: goto keel__m0_A;
    case app_mf_Cycle_B: goto keel__m0_B;
    default:             KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 10 "app/mf.k"
    keel__m0_A: {
        r = 1;
    }
    goto keel__m0_end;
#line 12 "app/mf.k"
    keel__m0_B: {
        r = 2;
    }
    keel__m0_end: ;
#line 15 "app/mf.k"
    return r;
}

int app_mf_inline_arms(void) {
    keel_tagged_app_mf_Cycle_void s = {0};
    int r = 0;
    keel_tagged_app_mf_Cycle_void_mark(&s, app_mf_Cycle_B);
    switch (s.tag) {
    case app_mf_Cycle_A: goto keel__m0_A;
    case app_mf_Cycle_B: goto keel__m0_B;
    default:             KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 23 "app/mf.k"
    keel__m0_A: { r = 1; }
    goto keel__m0_end;
#line 24 "app/mf.k"
    keel__m0_B: { r = 2; }
    keel__m0_end: ;
    return r;
}

int app_mf_with_loop(int n) {
    keel_tagged_app_mf_Cycle_void s = {0};
    int r = 0;
    switch (s.tag) {
    case app_mf_Cycle_A: goto keel__m0_A;
    case app_mf_Cycle_B: goto keel__m0_B;
    default:             KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 33 "app/mf.k"
    keel__m0_A: {
        for (int i = 0; i < n; i++) {
            if (i == 3) break;
            r += i;
        }
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 39 "app/mf.k"
    keel__m0_B: {
        r = 100;
    }
    keel__m0_end: ;
#line 42 "app/mf.k"
    return r + 1;
}
