/* proof.c — harness. Partitionable over a by-value container (keel.slice, not
   byref): the partition call must receive a value, not keel__c<N> itself —
   the lowering holds the container in a pointer temp regardless (backend
   §5.9), so passing it straight through would mismatch the parameter type. */

#include "app/app_ppv.h"

int main(void) {
    i32 ok[4] = {1, 2, 3, 4};
    if (!app_ppv_all_non_negative(ok, 4)) return 1;
    i32 bad[4] = {1, 2, -3, 4};
    if (app_ppv_all_non_negative(bad, 4)) return 2;
    return 0;
}
