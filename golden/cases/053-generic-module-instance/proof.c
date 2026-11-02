/* proof.c — harness. The constant and the type that cite only the module's parameters are in
   the header of the module's instance, with the arguments in the name; the header of the
   modifier's instance includes it. */

#include "keel.type.h"
#include "gd_box_3_i32.h"

int main(void) {
    gd_box_3_i32 b = { { 1, 2, 3 } };
    gd_3_i32_Pair p = { 1, 2 };
    if (gd_box_3_i32_cap(&b) != 3) return 1;
    if (gd_3_i32_CAP != 3) return 2;
    if (p.first + p.last != 3) return 3;
    return 0;
}
