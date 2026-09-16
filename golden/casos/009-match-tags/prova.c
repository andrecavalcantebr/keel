/* prova.c — arnês. O despacho é por etiqueta, o laço é daqui, e o que
   atravessa braços mora na struct do agente. */

#include "keel/prelude.h"
#include "ag/ag.impl.h"
#include "keel/corot.impl.h"
#include "keel/keel_tagged_ag_Ciclo_void.type.h"
#include <stdio.h>
int main(void) {
    keel_tagged_ag_Ciclo_void st = { ag_Ciclo_ST1 };
    ag_Agente ag = { 0 };
    i32 a = 2;

    keel_corot r = ag_passo(&a, &st, &ag);      /* ST1: fn1 cede            */
    if (!keel_corot_ongoing(r)) return 1;
    if (st.tag != ag_Ciclo_ST1) return 2;

    r = ag_passo(&a, &st, &ag);                 /* ST1: fn1 cede de novo    */
    if (!keel_corot_ongoing(r)) return 3;

    r = ag_passo(&a, &st, &ag);                 /* ST1: fn1 vence → ST2     */
    if (!keel_corot_ongoing(r)) return 4;       /* o `break` sai do match   */
    if (st.tag != ag_Ciclo_ST2 || ag.n != 7) return 5;

    r = ag_passo(&a, &st, &ag);                 /* ST2 → ST3, sem cair nele */
    if (!keel_corot_ongoing(r)) return 6;
    if (st.tag != ag_Ciclo_ST3) return 7;

    r = ag_passo(&a, &st, &ag);                 /* ST3: vence               */
    if (!keel_corot_ok(r)) return 8;

    /* etiqueta fora da lista cai no `default:` e sai do despacho */
    st.tag = 99;
    r = ag_passo(&a, &st, &ag);
    if (!keel_corot_ongoing(r)) return 9;

    puts("ok");
    return 0;
}
