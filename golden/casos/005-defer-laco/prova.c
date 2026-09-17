/* prova.c — arnês. Afirma que o cleanup roda uma vez por iteração, em TODA
   saída do corpo: `continue`, `break` e `return`. Nada pode vazar. */

#include "keel/keel.type.h"
#include "app/app_lc.impl.h"
#include <stdio.h>
int main(void) {
    app_lc_zera();
    if (app_lc_roda(20) != -1) return 1;         /* sai por `return` em i=7 */
    if (app_lc_vivos_agora()  != 0) return 2;    /* nada vazou */
    if (app_lc_soltos_agora() != 8) return 3;    /* i=0..7 */

    app_lc_zera();
    if (app_lc_roda(3) != 1 + 2 + 3) return 4;   /* sai pelo fim do laço */
    if (app_lc_vivos_agora()  != 0) return 5;
    if (app_lc_soltos_agora() != 3) return 6;
    puts("ok");
    return 0;
}
