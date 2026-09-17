/* prova.c — arnês do caso. NÃO é saída do transpilador: é o programa que
   exercita a interface pública gerada e afirma o comportamento.
   Compila junto com esperado/<perfil>/app/pos.c. */

#include "keel/prelude.h"
#include "app/app_pos.impl.h"
#include "keel/keel_arena.impl.h"
#include "keel/keel_outcome_i32.impl.h"
#include <stdio.h>

int main(void) {
    unsigned char buf[256];
    keel_arena a;
    if (!keel_arena_from_array(&a, buf, sizeof buf)) return 90;

    /* quatro linhas: x = 0,1,2,3 — soma 6. `ativo` e `y` não entram na soma,
       só existem para provar que o campo compartilhado e a segunda coluna
       convivem sem interferir na leitura de `x`. */
    keel_outcome_i32 r = app_pos_somar_x(&a, 4);
    if (keel_outcome_i32_failed(r)) return 1;
    if (keel_outcome_i32_value(r) != 6) return 2;

    /* capacidade maior que a arena consegue dar: falha pelo canal normal */
    keel_outcome_i32 r2 = app_pos_somar_x(&a, 1000000);
    if (!keel_outcome_i32_failed(r2)) return 3;
    if (keel_outcome_i32_code(r2) != 1) return 4;

    puts("ok");
    return 0;
}
