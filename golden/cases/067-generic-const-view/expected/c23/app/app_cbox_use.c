/* app/app_cbox_use.c — generated from app/cbox_use.k by cgen, C23 profile. */
#include "app/app_cbox_use.h"
#include "app/app_cbox_box_i32.h"
#include "keel/keel_slice_const_i32.h"
#line 1 "app/cbox_use.k"




size_t app_cbox_use_probe_length(void) {
    i32 arr[3] = {10, 20, 30};
    app_cbox_box_i32 b = { arr, 3 };
    keel_slice_const_i32 v = app_cbox_box_i32_view(&b);
    return keel_slice_const_i32_length(v);
}

i32 app_cbox_use_probe_get(size_t i) {
    i32 arr[3] = {10, 20, 30};
    app_cbox_box_i32 b = { arr, 3 };
    keel_slice_const_i32 v = app_cbox_box_i32_view(&b);
    return keel_slice_const_i32_get(v, i);
}
