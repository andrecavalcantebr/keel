/* proof.c — the case's harness. NOT transpiler output: it is the program
   that exercises the generated public interface and asserts the behaviour.
   Compiled together with expected/<perfil>/app/pos.c. */

#include "keel.type.h"
#include "app/app_pos.h"
#include "keel/keel_arena.h"
#include <stdio.h>

int main(void) {
    unsigned char buf[256];
    keel_arena a;
    if (!keel_arena_from_array(&a, buf, sizeof buf)) return 90;

    app_pos_position p;
    app_pos_reserve(&p, &a, 4);

    /* four rows: x = 0,1,2,3 — sum 6. y exists only to prove that the second
       column and the `active` field coexist without disturbing `x`. */
    if (!app_pos_push_row(&p, 0.0f, 0.0f)) return 1;
    if (!app_pos_push_row(&p, 1.0f, 2.0f)) return 2;
    if (!app_pos_push_row(&p, 2.0f, 4.0f)) return 3;
    if (!app_pos_push_row(&p, 3.0f, 6.0f)) return 4;

    /* cheia: cap == 4, a quinta falha pelo canal normal, sem keel envolvido */
    if (app_pos_push_row(&p, 4.0f, 8.0f)) return 5;

    if (app_pos_sum_x(&p) != 6.0f) return 6;

    puts("ok");
    return 0;
}
