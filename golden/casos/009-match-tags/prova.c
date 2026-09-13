/* prova.c — arnês. A máquina cede enquanto `fn1` cede, e o estado avança por
   atribuição do usuário; o que atravessa estados mora na struct do agente. */
#include "ag/ag.h"
#include <stdio.h>

int main(void) {
    ag_Agente ag = { ag_ciclo_ST1, 0 };
    int a = 2;
    keel_corot_i32 r = ag_passo(&a, &ag);
    if (!keel_corot_i32_ongoing(r)) return 1;
    r = ag_passo(&a, &ag);
    if (!keel_corot_i32_ongoing(r)) return 2;
    r = ag_passo(&a, &ag);                     /* fn1 vence → ST2 → ST3 → cowin */
    if (!keel_corot_i32_ok(r)) return 3;
    if (keel_corot_i32_value(r) != 7) return 4;
    puts("ok");
    return 0;
}
