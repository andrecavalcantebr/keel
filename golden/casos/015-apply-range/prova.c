/* prova.c — arnês do `apply` e do `range` nomeado. */
#include "app/ap.h"
#include <stdio.h>

int main(void) {
    i32 v[4] = {1,2,3,4};
    keel_buffer_i32 b = keel_buffer_i32_of(v, 4);
    app_ap_percorre(&b);
    /* dobra: (2+0)+(4+1)+(6+2)+(8+3)=26 ; escala: (3+0)+(6+1)+(9+2)+(12+3)=36 */
    if (app_ap_soma_acc() != 26 + 36) return 1;

    keel_range r = keel_range_of(2, 6);          /* [2,6) → 2,3,4,5 */
    /* soma = 14 ; ponderada = 2*0+3*1+4*2+5*3 = 26 */
    if (app_ap_soma_range(r) != 14 + 26) return 2;
    puts("ok");
    return 0;
}
