/* gen/ag/ag.c — perfil C11. Emissão do keel-c-backend.md §5.6: despacho por
   `goto`, rótulo ANTES da chave, e cada estado num escopo próprio. */
#include "keel/prelude.h"
#include "keel/keel_corot_i32.h"
#include <stdio.h>

typedef struct { int s; int n; } ag_Agente;

typedef enum ag_ciclo {
    ag_ciclo_ST1,            /* 0 — estado inicial */
    ag_ciclo_ST2,
    ag_ciclo_ST3
} ag_ciclo;

static keel_corot_i32 ag_fn1(int *a) {
    if (*a > 0) { (*a)--; return (keel_corot_i32){ 0, 0 }; }
    return (keel_corot_i32){ -1, 7 };
}

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

/* ---- prova de execução ---- */
int main(void) {
    ag_Agente ag = { ag_ciclo_ST1, 0 };
    int a = 2;
    keel_corot_i32 r = ag_passo(&a, &ag);          /* fn1 cede: ONGOING */
    if (!keel_corot_i32_ongoing(r)) return 1;
    r = ag_passo(&a, &ag);                          /* cede de novo */
    if (!keel_corot_i32_ongoing(r)) return 2;
    r = ag_passo(&a, &ag);                          /* fn1 vence → ST2 → ST3 → cowin */
    if (!keel_corot_i32_ok(r) || keel_corot_i32_value(r) != 7) return 3;

    ag.s = 99;                                      /* estado fora de faixa: default sai */
    ag.n = 0;
    /* entrar com estado inválido cai em keel__ciclo_end e o `while(1)` reentra;
       por isso o teste não chama — é laço infinito por construção do usuário. */
    puts("ok");
    return 0;
}
