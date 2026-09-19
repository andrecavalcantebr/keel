/* ag.c — gerado de ag/ag.k, perfil C11.
   Despacho por `goto` e blocos rotulados, nunca um `switch` com o corpo do
   usuário dentro. O rótulo vai ANTES da chave, e as chaves são o escopo do
   braço que a linguagem §4.9 exige (backend §5.6, regra 2). */

#include "keel.type.h"
#include "ag.h"
#include "keel/keel_corot.h"
#include "keel/keel_tagged_ag_Ciclo_void.h"
#line 9 "ag/ag.k"
static keel_corot ag_fn1(i32 *a) {
    keel_corot r = {0};
    if (*a > 0) { (*a)--; return keel_corot_again(&r); }
    return keel_corot_win(&r);
}

#line 18 "ag/ag.k"
keel_corot ag_passo(i32 *a, keel_tagged_ag_Ciclo_void *st, ag_Agente *ag) {
    keel_corot out = {0};
    switch (st->tag) {                                /* só saltos: nada do usuário aqui */
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
        goto keel__m0_end;                            /* o `break` do braço */
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
    return keel_corot_again(&out);
}
