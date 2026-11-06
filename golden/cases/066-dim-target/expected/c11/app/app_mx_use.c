/* app/app_mx_use.c — generated from app/mx_use.k by cgen, C11 profile. */
#include "app/app_mx_use.h"
#include "mx_box_2_3.h"
#line 1 "app/mx_use.k"



int app_mx_use_main(void) {
    mx_box_2_3 v = mx_box_2_3_zeros();
    mx_box_2_3_set(&v, 1, 2, 5.0);
    if (mx_box_2_3_get(&v, 0, 0) != 0.0) return 1;
    if (mx_box_2_3_get(&v, 1, 2) != 5.0) return 1;
    return 0;
}
