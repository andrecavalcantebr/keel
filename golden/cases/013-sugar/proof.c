/* proof.c — harness for the sugar. The second test is the one that matters:
   the index must be evaluated ONCE, which the lowering by function guarantees
   and a lowering by macro would not. */

#include "keel.type.h"
#include "app/app_ac.h"
#include "keel/keel_buffer_i32.h"
#include <stdio.h>
int main(void) {
    i32 v[10];
    for (size_t k = 0; k < 10; k++) v[k] = (i32)k;
    keel_buffer_i32 b = keel_buffer_i32_of(v, 10);

    if (app_ac_bump(&b, 3) != 4) return 1;    /* v[3] era 3, virou 4 */
    size_t i = 2;
    if (app_ac_once(&b, &i) != 3) return 2; /* i incrementado UMA vez */
    if (v[2] != 9) return 3;
    if (app_ac_cube() != 7) return 4;
    if (app_ac_cut(&b) != 5 + 5 + 3 + 10) return 5;
    puts("ok");
    return 0;
}
