/* gen/ag/ag.c — gerado de ag/ag.k, perfil C23.
   Despacho por `goto` e blocos rotulados, nunca um `switch` com o corpo do
   usuário dentro. O rótulo vai ANTES da chave, e as chaves são o escopo do
   estado que a linguagem §4.8 exige (backend §5.6, regra 2). */
#include "ag/ag.h"

#line 10 "ag/ag.k"
static keel_corot_i32 ag_fn1(int *a) {
    if (*a > 0) { (*a)--; return (keel_corot_i32){ 0, 0 }; }
    return (keel_corot_i32){ -1, 7 };
}

#line 17 "ag/ag.k"
keel_corot_i32 ag_passo(int *a, ag_Agente *ag) {
    while (1) {
    switch (ag->s) {                                  /* só saltos: nada do usuário aqui */
    case ag_ciclo_ST1: goto keel__ciclo_ST1;
    case ag_ciclo_ST2: goto keel__ciclo_ST2;
    case ag_ciclo_ST3: goto keel__ciclo_ST3;
    default:           goto keel__ciclo_end;
    }
    keel__ciclo_ST1: {
        keel_corot_i32 r = ag_fn1(a);
        if (r.code > 0) return (keel_corot_i32){ 1, 0 };
        if (r.code == 0) return (keel_corot_i32){ 0, 0 };
        ag->n = r.v;
        ag->s = ag_ciclo_ST2; goto keel__ciclo_end;
    }
    goto keel__ciclo_end;
    keel__ciclo_ST2: {
        if (ag->n >= 20) return (keel_corot_i32){ 0, 0 };
        ag->s = ag_ciclo_ST3;
    }
    goto keel__ciclo_end;
    keel__ciclo_ST3: {
        return (keel_corot_i32){ -1, ag->n };
    }
    keel__ciclo_end: ;
    }
}
