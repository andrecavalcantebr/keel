/* app/app_ibox_use.c — generated from app/ibox_use.k by cgen, C11 profile. */
#include "app/app_ibox_use.h"
#include "app/app_ibox_use_count_app_ibox_box_i32.h"
#line 1 "app/ibox_use.k"





size_t app_ibox_use_probe_count(void) {
    i32 arr[3] = {10, 20, 30};
    app_ibox_box_i32 b = { arr, 3 };
    return app_ibox_use_count_app_ibox_box_i32(b);
}
