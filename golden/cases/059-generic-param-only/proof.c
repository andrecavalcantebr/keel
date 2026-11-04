/* proof.c — harness. The client reaches the instance box(3) i32 only through its public
   functions: cap is the constant of the generic, and ends returns the pair of extremes. */

#include "app/app_pbox_use.h"

int main(void) {
    if (app_pbox_use_probe_cap() != 3) return 1;
    if (app_pbox_use_probe_first() != 4) return 2;
    if (app_pbox_use_probe_last() != 6) return 3;
    return 0;
}
