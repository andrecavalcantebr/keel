/* proof.c — harness. Each line checks one form of ar.k: the index of an array field
   (literal and checked), the verbs of `keel` over local, module and binder arrays, and a
   declaration with two declarators. */

#include "keel.type.h"
#include "app/app_ar.h"

int main(void) {
    app_ar_Box b = { { 1, 2, 3, 4 } };
    app_ar_Grid gr = { { { 1, 2, 3 }, { 4, 5, 6 } } };
    i32 five[5] = { 0 };
    if (app_ar_at_lit(&b) != 3 || app_ar_at_val(b) != 3) return 1;
    app_ar_put(&b, 1);
    if (b.v[1] != 1) return 2;
    if (app_ar_cell(&gr, 1, 2) != 6 || app_ar_cell_lit(&gr) != 6) return 3;
    if (app_ar_len_local() != 15) return 4;              /* 6 + 3 + 6 */
    if (app_ar_dim_mod() != 4) return 5;
    if (app_ar_len_param(five, 5) != 10) return 6;       /* the binder twice */
    if (app_ar_pair() != 3) return 7;
    return 0;
}
