/* proof.c — harness. Neither A, B nor D is defined, so the compiler takes the
   last alternative of every group: the braces and parens the cgen counted
   are the ones it pairs. */

#include "keel.type.h"
#include "app/app_dag.h"

int main(void) {
    if (app_dag_crosses(3) != 4) return 1;  /* x>1: x++ before goto out */
    if (app_dag_crosses(0) != 0) return 2;
    if (app_dag_wrapped(5) != 7) return 3;
    return 0;
}
