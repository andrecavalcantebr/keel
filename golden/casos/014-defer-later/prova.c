/* prova.c — arnês. A diferença entre `later` e `[now]` é SEMÂNTICA, e é isso
   que este arnês afirma: o mesmo `defer` sobre o mesmo código dá 3 e 1. */

#include "keel/prelude.h"
#include "app/dl.impl.h"
#include <stdio.h>
int main(void) {
    app_dl_tarde();          if (app_dl_ultimo() != 3) return 1;  /* valor na saída */
    app_dl_cedo();           if (app_dl_ultimo() != 1) return 2;  /* valor no registro */
    app_dl_tarde_escrito();  if (app_dl_ultimo() != 7) return 3;  /* `later` == padrão */
    puts("ok");
    return 0;
}
