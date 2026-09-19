/* prova.c — arnês do caso. NÃO é saída do transpilador: é o programa que
   exercita a interface pública gerada e afirma o comportamento.
   Compila junto com esperado/<perfil>/app/cfg.c. */

#include "keel.type.h"
#include "app/app_cfg.h"
#include "keel/keel_outcome_i32.h"
#include <stdio.h>
int main(void) {
    /* escreve um arquivo com quatro inteiros e soma pela API gerada */
    FILE *f = fopen("/tmp/keel_c001.txt", "w");
    if (!f) return 90;
    fputs("1 2 3 4\n", f); fclose(f);

    keel_outcome_i32 r = app_cfg_soma_scratch("/tmp/keel_c001.txt");
    if (keel_outcome_i32_failed(r)) return 1;
    if (keel_outcome_i32_value(r) != 10) return 2;

    /* arquivo inexistente: falha com o código 1, e o defer fecha nada */
    r = app_cfg_soma_scratch("/tmp/keel_c001_nao_existe.txt");
    if (!keel_outcome_i32_failed(r)) return 3;
    if (keel_outcome_i32_code(r) != 1) return 4;

    puts("ok");
    return 0;
}
