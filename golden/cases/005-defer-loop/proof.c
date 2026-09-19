/* proof.c — harness. Asserts that the cleanup runs once per iteration, at
   EVERY exit of the body: `continue`, `break` and `return`. Nothing may leak. */

#include "keel.type.h"
#include "app/app_loop.h"
#include <stdio.h>
int main(void) {
    app_loop_reset();
    if (app_loop_run(20) != -1) return 1;         /* sai por `return` em i=7 */
    if (app_loop_live_now()  != 0) return 2;    /* nada vazou */
    if (app_loop_released_now() != 8) return 3;    /* i=0..7 */

    app_loop_reset();
    if (app_loop_run(3) != 1 + 2 + 3) return 4;   /* leaves through the end of the loop */
    if (app_loop_live_now()  != 0) return 5;
    if (app_loop_released_now() != 3) return 6;
    puts("ok");
    return 0;
}
