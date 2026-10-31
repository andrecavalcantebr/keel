/* mc.c — generated from mc.k by cgen, C11 profile. */
#include "mc.h"
#include "keel/keel_corot.h"
#line 1 "mc.k"






i32 mc_classify(keel_corot r) {
    i32 n = 0;
    switch (keel_corot_tag(r)) {
    case keel_corot_Status_SUCCESS: goto keel__m0_SUCCESS;
    case keel_corot_Status_ONGOING: goto keel__m0_ONGOING;
    case keel_corot_Status_FAILED:  goto keel__m0_FAILED;
    default:                        KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 10 "mc.k"
    keel__m0_SUCCESS: {
        n = 1;
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 13 "mc.k"
    keel__m0_ONGOING: {
        n = 2;
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 16 "mc.k"
    keel__m0_FAILED: {
        n = keel_corot_code(r);
    }
    keel__m0_end: ;
#line 19 "mc.k"
    return n;
}


i32 mc_rank(keel_corot_Status s) {
    i32 n = 0;
    switch (s) {
    case keel_corot_Status_SUCCESS: goto keel__m0_SUCCESS;
    case keel_corot_Status_ONGOING: goto keel__m0_ONGOING;
    case keel_corot_Status_FAILED:  goto keel__m0_FAILED;
    default:                        KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 26 "mc.k"
    keel__m0_SUCCESS: {
        n = -1;
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 29 "mc.k"
    keel__m0_ONGOING: {
        n = 0;
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 32 "mc.k"
    keel__m0_FAILED: {
        n = 1;
    }
    keel__m0_end: ;
#line 35 "mc.k"
    return n;
}



i32 mc_classify_ptr(keel_corot *r) {
    i32 n = 0;
    switch (keel_corot_tag(*r)) {
    case keel_corot_Status_SUCCESS: goto keel__m0_SUCCESS;
    case keel_corot_Status_ONGOING: goto keel__m0_ONGOING;
    case keel_corot_Status_FAILED:  goto keel__m0_FAILED;
    default:                        KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 43 "mc.k"
    keel__m0_SUCCESS: {
        n = 1;
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 46 "mc.k"
    keel__m0_ONGOING: {
        n = 2;
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 49 "mc.k"
    keel__m0_FAILED: {
        n = keel_corot_code(*r);
    }
    keel__m0_end: ;
#line 52 "mc.k"
    return n;
}

i32 mc_rank_ptr(keel_corot_Status *s) {
    i32 n = 0;
    switch (*s) {
    case keel_corot_Status_SUCCESS: goto keel__m0_SUCCESS;
    case keel_corot_Status_ONGOING: goto keel__m0_ONGOING;
    case keel_corot_Status_FAILED:  goto keel__m0_FAILED;
    default:                        KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 58 "mc.k"
    keel__m0_SUCCESS: {
        n = -1;
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 61 "mc.k"
    keel__m0_ONGOING: {
        n = 0;
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 64 "mc.k"
    keel__m0_FAILED: {
        n = 1;
    }
    keel__m0_end: ;
#line 67 "mc.k"
    return n;
}
