/* proof.c — harness. The constexpr chain (each INIT cites the previous one), the function
   that returns the last one, the walk over a list whose cursor is written qualified, and the
   foreach with the Allman braces, over a slice passed by value. */

#include "keel.type.h"
#include "keel/keel_slice_i32.h"
#include "app/app_sf.h"
#include <assert.h>

int main(void) {
    assert(app_sf_A == 2 && app_sf_B == 4 && app_sf_C == 6);
    assert(app_sf_cval() == 6);

    app_sf_Node c3 = { 3, NULL }, b2 = { 4, &c3 }, a1 = { 1, &b2 };
    app_sf_List l = { &a1 };
    assert(app_sf_sum(&l) == 8);

    app_sf_List empty = { NULL };
    assert(app_sf_sum(&empty) == 0);

    i32 v[3] = { 5, 6, 7 };
    assert(app_sf_allman(keel_slice_i32_from(v, 3)) == 15);
    assert(app_sf_allman(keel_slice_i32_from(v, 0)) == 0);
    return 0;
}
