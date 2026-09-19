/* proof.c — harness. Dispatch is by tag, the loop belongs here, and what
   crosses arms lives in the agent's struct. */

#include "keel.type.h"
#include "ag.h"
#include "keel/keel_corot.h"
#include "keel/keel_tagged_ag_Cycle_void.type.h"
#include <stdio.h>
int main(void) {
    keel_tagged_ag_Cycle_void st = { ag_Cycle_ST1 };
    ag_Agent ag = { 0 };
    i32 a = 2;

    keel_corot r = ag_step(&a, &st, &ag);      /* ST1: fn1 cede            */
    if (!keel_corot_ongoing(r)) return 1;
    if (st.tag != ag_Cycle_ST1) return 2;

    r = ag_step(&a, &st, &ag);                 /* ST1: fn1 cede de novo    */
    if (!keel_corot_ongoing(r)) return 3;

    r = ag_step(&a, &st, &ag);                 /* ST1: fn1 vence → ST2     */
    if (!keel_corot_ongoing(r)) return 4;       /* o `break` sai do match   */
    if (st.tag != ag_Cycle_ST2 || ag.n != 7) return 5;

    r = ag_step(&a, &st, &ag);                 /* ST2 → ST3, sem cair nele */
    if (!keel_corot_ongoing(r)) return 6;
    if (st.tag != ag_Cycle_ST3) return 7;

    r = ag_step(&a, &st, &ag);                 /* ST3: vence               */
    if (!keel_corot_ok(r)) return 8;

    /* etiqueta fora da lista cai no `default:` e sai do despacho */
    st.tag = 99;
    r = ag_step(&a, &st, &ag);
    if (!keel_corot_ongoing(r)) return 9;

    puts("ok");
    return 0;
}
