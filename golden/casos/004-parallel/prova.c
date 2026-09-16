/* prova.c — arnês. É regressão do rótulo de término: com a emissão anterior,
   que punha o rótulo ANTES da gravação de SUCCESS, o `fail` era apagado e o
   segundo teste devolvia falso. */

#include "keel/prelude.h"
#include "app/val.impl.h"
#include "keel/keel_buffer_i32.impl.h"
#include <stdio.h>
int main(void) {
    i32 v[16];
    keel_buffer_i32 b = keel_buffer_i32_of(v, 16);
    for (size_t i = 0; i < 16; i++) keel_buffer_i32_set(&b, i, 1);

    if (app_val_tem_negativo(&b)) return 1;      /* nenhum negativo */
    keel_buffer_i32_set(&b, 13, -7);             /* faixa 3 */
    if (!app_val_tem_negativo(&b)) return 2;     /* o `fail` tem de sobreviver */
    puts("ok");
    return 0;
}
