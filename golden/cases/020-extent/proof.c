/* proof.c — the case's harness. NOT transpiler output: it is the program
   that exercises the generated public interface and asserts the behaviour.
   Compiled together with expected/<perfil>/app/pos.c. */

#include "keel.type.h"
#include "app/app_pos.h"
#include "keel/keel_arena.h"
#include <stdio.h>

int main(void) {
    unsigned char buf[512];
    keel_arena a;
    if (!keel_arena_from_array(&a, buf, sizeof buf)) return 90;

    /* four rows: x = 0,1,2,3 — sum 6. y exists only to prove that the second
       column and the `active` field coexist without disturbing `x`. */
    struct app_pos_position p;
    if (!app_pos_reserve(&p, &a, 4)) return 91;
    if (!app_pos_push_row(&p, 0.0f, 0.0f)) return 1;
    if (!app_pos_push_row(&p, 1.0f, 2.0f)) return 2;
    if (!app_pos_push_row(&p, 2.0f, 4.0f)) return 3;
    if (!app_pos_push_row(&p, 3.0f, 6.0f)) return 4;

    /* full: len == cap == 4, the fifth fails through the normal channel */
    if (app_pos_push_row(&p, 4.0f, 8.0f)) return 5;
    if (app_pos_sum_x(&p) != 6.0f) return 6;

    /* 2 x 3 rows written under a 3 x 4 capacity. The stride is the inner
       capacity (4), not the count (3): row 1 starts at v[4]. */
    struct app_pos_grid g;
    if (!app_pos_grid_init(&g, &a, 3, 4)) return 92;
    app_pos_grid_fill(&g, 2, 3);
    if (app_pos_grid_at(&g, 1, 2) != 12) return 7;
    if (g.v[4] != 10) return 8;

    /* embedded columns: plain C 2-D arrays, i * W + j */
    struct app_pos_image img;
    app_pos_paint(&img);
    if (img.r[1][2] != 5) return 9;
    if (img.g[1][2] != 250) return 10;

    puts("ok");
    return 0;
}
