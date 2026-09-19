/* proof.c — the case's harness. NOT transpiler output: it is the program
   that exercises the generated public interface and asserts the behaviour.
   Compiled together with expected/<perfil>/app/cfg.c. */

#include "keel.type.h"
#include "app/app_cfg.h"
#include "keel/keel_outcome_i32.h"
#include <stdio.h>
int main(void) {
    /* escreve um arquivo com quatro inteiros e soma pela API gerada */
    FILE *f = fopen("/tmp/keel_c001.txt", "w");
    if (!f) return 90;
    fputs("1 2 3 4\n", f); fclose(f);

    keel_outcome_i32 r = app_cfg_sum_scratch("/tmp/keel_c001.txt");
    if (keel_outcome_i32_failed(r)) return 1;
    if (keel_outcome_i32_value(r) != 10) return 2;

    /* missing file: fails with code 1, and the defer closes nothing */
    r = app_cfg_sum_scratch("/tmp/keel_c001_does_not_exist.txt");
    if (!keel_outcome_i32_failed(r)) return 3;
    if (keel_outcome_i32_code(r) != 1) return 4;

    puts("ok");
    return 0;
}
