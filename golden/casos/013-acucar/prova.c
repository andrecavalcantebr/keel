/* prova.c — arnês do açúcar. O segundo teste é o que importa: o índice tem de
   ser avaliado UMA vez, que é o que o lowering por função garante e o por
   macro não garantiria. */

#include "keel/prelude.h"
#include "app/ac.impl.h"
#include "keel/keel_buffer_i32.impl.h"
#include <stdio.h>
int main(void) {
    i32 v[10];
    for (size_t k = 0; k < 10; k++) v[k] = (i32)k;
    keel_buffer_i32 b = keel_buffer_i32_of(v, 10);

    if (app_ac_troca(&b, 3) != 4) return 1;    /* v[3] era 3, virou 4 */
    size_t i = 2;
    if (app_ac_uma_vez(&b, &i) != 3) return 2; /* i incrementado UMA vez */
    if (v[2] != 9) return 3;
    if (app_ac_cubo() != 7) return 4;
    if (app_ac_recorta(&b) != 5 + 5 + 3 + 10) return 5;
    puts("ok");
    return 0;
}
