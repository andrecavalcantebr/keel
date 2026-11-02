/* proof.c — harness. The cleanups run after the body, in reverse order of
   registration, and they read `v` at the exit: `seen` is registered first, so it
   runs last and records what the other cleanups left in the array. */

#include "keel.type.h"
#include "app/app_di.h"
#include <stdio.h>

int main(void) {
    app_di_clear_var(0);
    if (app_di_last() != 0) return 1;      /* v[0] zeroed by the cleanup, then seen = 0 */
    app_di_clear_var(2);
    if (app_di_last() != 7) return 2;      /* v[2] zeroed; v[0] still 7 */

    if (app_di_take_var(0) != 7) return 3; /* the value is read BEFORE the cleanup */
    if (app_di_last() != 0) return 4;      /* and the cleanup zeroed v[0] */
    if (app_di_take_var(2) != 7) return 5;
    if (app_di_last() != 7) return 6;

    app_di_clear_now();
    if (app_di_last() != 0) return 7;      /* v[1] zeroed: the copy of k was 1, not 2 */

    app_di_clear_lit();
    if (app_di_last() != 70) return 8;     /* v[0] = 7, v[1] zeroed: 7*10 + 0 */

    app_di_two_vars(0, 2);
    if (app_di_last() != 182) return 9;    /* {1,8,2} */
    app_di_two_vars(1, 2);
    if (app_di_last() != 712) return 10;   /* {7,1,2} */
    app_di_two_vars(1, 1);
    if (app_di_last() != 729) return 11;   /* v[1] = 1, then = 2: {7,2,9} */

    puts("ok");
    return 0;
}
