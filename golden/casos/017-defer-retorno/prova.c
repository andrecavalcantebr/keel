/* prova.c — arnês. A ordem é o que importa: `expr` é avaliada ANTES do
   cleanup, e o cleanup zera o campo que a expressão lê. Devolver 0 em vez de 5
   denunciaria a ordem errada, e nenhuma compilação acusaria isso. */

#include "keel/prelude.h"
#include "app/dr.impl.h"
#include <stdio.h>
int main(void) {
    app_dr_Rec r = { 5 };
    if (app_dr_consome(&r, 0) != 5) return 1;   /* valor ANTES do cleanup */
    if (r.v != 0) return 2;                     /* e o cleanup rodou */

    r.v = 5;
    if (app_dr_consome(&r, 1) != 10) return 3;
    if (r.v != 0) return 4;

    r.v = 0;
    if (app_dr_dois(&r, 1) != -1) return 5;
    if (r.v != 1) return 6;                     /* só marca_a: 0*10+1 */

    r.v = 0;
    if (app_dr_dois(&r, 0) != -2) return 7;
    if (r.v != 21) return 8;                    /* b depois a: (0*10+2)*10+1 */

    r.v = 1;
    app_dr_nada(&r);
    if (r.v != 0) return 9;
    puts("ok");
    return 0;
}
