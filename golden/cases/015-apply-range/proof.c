/* proof.c — harness for `apply` and for the named `range`. */

#include "keel.type.h"
#include "app/app_ap.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_range.h"
#include <stdio.h>
int main(void) {
    i32 v[4] = {1,2,3,4};
    keel_buffer_i32 b = keel_buffer_i32_of(v, 4);
    app_ap_traverse(&b);
    /* dobra: (2+0)+(4+1)+(6+2)+(8+3)=26 ; escala: (3+0)+(6+1)+(9+2)+(12+3)=36 */
    if (app_ap_sum_acc() != 26 + 36) return 1;

    keel_range r = keel_range_of(2, 6);          /* [2,6) → 2,3,4,5 */
    /* soma = 14 ; ponderada = 2*0+3*1+4*2+5*3 = 26 */
    if (app_ap_sum_range(r) != 14 + 26) return 2;
    puts("ok");
    return 0;
}
