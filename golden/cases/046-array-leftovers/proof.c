/* proof.c — harness. The open ranges of an array (4 + 6 elements), two arrays declared together
   in a struct, the extents of parameters (with and without binder), and the length of an array
   of a type of the module. */

#include "keel.type.h"
#include "app/app_al.h"

int main(void) {
    app_al_Pair p = { { 1, 2 }, { 3, 4, 5 } };
    i32 five[5] = { 0 };
    i32 m[2][3] = { { 0 } };
    if (app_al_from_two() != 10) return 1;
    if (app_al_pair(&p) != 5) return 2;
    if (app_al_cap(five, 5) != 5) return 3;
    if (app_al_plain(m) != 6) return 4;
    if (app_al_boxes() != 3) return 5;
    return 0;
}
