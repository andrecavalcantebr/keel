/* app/app_rg.c — generated from app/rg.k by cgen, C11 profile. */
#include "app/app_rg.h"
#include "keel/keel_array_i32.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/rg.k"




static i32 app_rg_p[3] = {1, 2, 3}, app_rg_q[2] = {4, 5};

size_t app_rg_window(void) {
    i32 v[6] = {1, 2, 3, 4, 5, 6};
    keel_slice_i32 s = keel_array_i32_as_slice2(v, 6, 1, 4);
    keel_slice_i32 t = keel_array_i32_as_slice2(v, 6, 0, 3);
    return keel_slice_i32_length(s) + keel_slice_i32_length(t) + sizeof(app_rg_p)/sizeof(i32) + sizeof(app_rg_q)/sizeof(i32);
}
