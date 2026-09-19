/* proof.c — harness. The order is what matters: `expr` is evaluated BEFORE
   the cleanup, and the cleanup zeroes the field the expression reads.
   Returning 0 instead of 5 would betray the wrong order, and no compilation
   would catch it. */

#include "keel.type.h"
#include "app/app_dr.h"
#include <stdio.h>
int main(void) {
    app_dr_Rec r = { 5 };
    if (app_dr_consume(&r, 0) != 5) return 1;   /* valor ANTES do cleanup */
    if (r.v != 0) return 2;                     /* e o cleanup rodou */

    r.v = 5;
    if (app_dr_consume(&r, 1) != 10) return 3;
    if (r.v != 0) return 4;

    r.v = 0;
    if (app_dr_two(&r, 1) != -1) return 5;
    if (r.v != 1) return 6;                     /* only mark_a: 0*10+1 */

    r.v = 0;
    if (app_dr_two(&r, 0) != -2) return 7;
    if (r.v != 21) return 8;                    /* b then a: (0*10+2)*10+1 */

    r.v = 1;
    app_dr_nothing(&r);
    if (r.v != 0) return 9;
    puts("ok");
    return 0;
}
