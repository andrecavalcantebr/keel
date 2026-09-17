/* prova.c — arnês do caso. NÃO é saída do transpilador: é o programa que
   exercita a interface pública gerada e afirma o comportamento.
   Compila junto com esperado/<perfil>/app/pos.c. */

#include "keel/prelude.h"
#include "app/app_pos.impl.h"
#include "keel/keel_arena.impl.h"
#include <stdio.h>

int main(void) {
    unsigned char buf[256];
    keel_arena a;
    if (!keel_arena_from_array(&a, buf, sizeof buf)) return 90;

    app_pos_position p;
    app_pos_ocupar(&p, &a, 4);

    /* quatro linhas: x = 0,1,2,3 — soma 6. y só existe para provar que a
       segunda coluna e o campo `ativo` convivem sem interferir em `x`. */
    if (!app_pos_linha(&p, 0.0f, 0.0f)) return 1;
    if (!app_pos_linha(&p, 1.0f, 2.0f)) return 2;
    if (!app_pos_linha(&p, 2.0f, 4.0f)) return 3;
    if (!app_pos_linha(&p, 3.0f, 6.0f)) return 4;

    /* cheia: cap == 4, a quinta falha pelo canal normal, sem keel envolvido */
    if (app_pos_linha(&p, 4.0f, 8.0f)) return 5;

    if (app_pos_somar_x(&p) != 6.0f) return 6;

    puts("ok");
    return 0;
}
