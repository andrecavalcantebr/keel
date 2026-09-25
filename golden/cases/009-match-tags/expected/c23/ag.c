/* ag.c — generated from ag.k by cgen, C23 profile. */
#include "ag.h"
#include "keel/keel_corot.h"
#include "keel/keel_tagged_ag_Cycle_void.h"
#line 1 "ag.k"









static keel_corot ag_fn1(i32 *a) {
    keel_corot r = {0};
    if (*a > 0) { (*a)--; return keel_corot_again(&r); }
    return keel_corot_win(&r);
}



keel_corot ag_step(i32 *a, keel_tagged_ag_Cycle_void *st, ag_Agent *ag) {
    keel_corot out = {0};
    switch (st->tag) {
    case ag_Cycle_ST1: goto keel__m0_ST1;
    case ag_Cycle_ST2: goto keel__m0_ST2;
    case ag_Cycle_ST3: goto keel__m0_ST3;
    default:           KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
    }
#line 21 "ag.k"
    keel__m0_ST1: {
        keel_corot r = ag_fn1(a);
        if (keel_corot_faulted(r)) return keel_corot_fault(&out, 1);
        if (keel_corot_ongoing(r)) return keel_corot_again(&out);
        ag->n = 7;
        keel_tagged_ag_Cycle_void_mark(st, ag_Cycle_ST2);
        goto keel__m0_end;
    }
    goto keel__m0_end;
#line 28 "ag.k"
    keel__m0_ST2: {
        if (ag->n >= 20) return keel_corot_again(&out);
        keel_tagged_ag_Cycle_void_mark(st, ag_Cycle_ST3);
    }
    goto keel__m0_end;
#line 31 "ag.k"
    keel__m0_ST3: {
        return keel_corot_win(&out);
    }
    keel__m0_end: ;
#line 34 "ag.k"
    return keel_corot_again(&out);
}
