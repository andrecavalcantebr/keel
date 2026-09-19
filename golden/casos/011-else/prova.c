/* prova.c — arnês das duas formas da cláusula `else`. */

#include "keel.type.h"
#include "app/app_el.h"
#include <stdio.h>
int main(void) {
    i32 base[4] = {1,2,3,4};
    if (app_el_dobro(5)  != 10) return 1;
    if (app_el_dobro(-1) != -1) return 2;      /* fail(7) → saída */
    if (app_el_dobro(0)  != -1) return 3;      /* NONE também é falha */
    if (app_el_dobro_log(3) != 6) return 4;
    if (app_el_tamanho(3, base) != 3) return 5;
    if (app_el_tamanho(0, base) != 0) return 6; /* default reparou o símbolo */
    puts("ok");
    return 0;
}
