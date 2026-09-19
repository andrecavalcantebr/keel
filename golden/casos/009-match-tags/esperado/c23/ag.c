/* ag.c — gerado de ag.k pelo cgen, perfil C23. */
#include "ag.h"
#include "keel/keel_corot.h"
#include "keel/keel_tagged_ag_Ciclo_void.h"
#line 10 "ag.k"
static keel_corot ag_fn1(i32 *a) {
    keel_corot r = {0};
    if (*a > 0) { (*a)--; return keel_corot_again(&r); }
    return keel_corot_win(&r);
}

#line 18 "ag.k"
keel_corot ag_passo(i32 *a, keel_tagged_ag_Ciclo_void *st, ag_Agente *ag) {
    keel_corot out = {0};
    switch (st->tag) {
    case ag_Ciclo_ST1: goto keel__m0_ST1;
    case ag_Ciclo_ST2: goto keel__m0_ST2;
    case ag_Ciclo_ST3: goto keel__m0_ST3;
    default:           goto keel__m0_end;
    }
    keel__m0_ST1: {
        keel_corot r = ag_fn1(a);
        if (keel_corot_faulted(r)) return keel_corot_fault(&out, 1);
        if (keel_corot_ongoing(r)) return keel_corot_again(&out);
        ag->n = 7;
        keel_tagged_ag_Ciclo_void_mark(st, ag_Ciclo_ST2);
        goto keel__m0_end;
    }
    goto keel__m0_end;
    keel__m0_ST2: {
        if (ag->n >= 20) return keel_corot_again(&out);
        keel_tagged_ag_Ciclo_void_mark(st, ag_Ciclo_ST3);
    }
    goto keel__m0_end;
    keel__m0_ST3: {
        return keel_corot_win(&out);
    }
    keel__m0_end: ;
#line 34 "ag.k"
    return keel_corot_again(&out);
}
