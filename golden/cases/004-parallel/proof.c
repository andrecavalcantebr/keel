/* proof.c — harness. It is a regression on the termination label: with the
   previous emission, which put the label BEFORE writing SUCCESS, the `fail`
   was erased and the second test returned false. */

#include "keel.type.h"
#include "app/app_val.h"
#include "keel/keel_buffer_i32.h"
#include <stdio.h>
int main(void) {
    i32 v[16];
    keel_buffer_i32 b = keel_buffer_i32_of(v, 16);
    for (size_t i = 0; i < 16; i++) keel_buffer_i32_set(&b, i, 1);

    if (app_val_has_negative(&b)) return 1;      /* nenhum negativo */
    keel_buffer_i32_set(&b, 13, -7);             /* faixa 3 */
    if (!app_val_has_negative(&b)) return 2;     /* o `fail` tem de sobreviver */
    puts("ok");
    return 0;
}
