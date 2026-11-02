/* proof.c — harness. The default of the `else` clause on the line after it: a failed view
   falls back to the empty slice, a good one keeps its length. */

#include "keel.type.h"
#include "app/app_ed.h"

int main(void) {
    i32 a[4] = { 1, 2, 3, 4 };
    if (app_ed_size(3, a) != 3) return 1;
    if (app_ed_size(0, a) != 0) return 2;
    return 0;
}
