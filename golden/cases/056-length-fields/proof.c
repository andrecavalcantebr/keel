/* proof.c — harness. The length of an array field, through a pointer and by value (5 + 5), the
   length of a module array whose dimension is a constexpr (3), the literal dimension 1 of a
   parameter without a binder (3), and the length of a range over a field (1..4: 3). */

#include "keel.type.h"
#include "app/app_lf.h"

int main(void) {
    app_lf_Box b = { { 1, 2, 3, 4, 5 } };
    i32 m[2][3] = { { 0 } };
    if (app_lf_field_ptr(&b) != 5) return 1;
    if (app_lf_field_val(b) != 5) return 2;
    if (app_lf_mod_len() != 3) return 3;
    if (app_lf_d(m) != 3) return 4;
    if (app_lf_field_slice(&b) != 3) return 5;
    return 0;
}
