/* app/app_mx_use.c — generated from app/mx_use.k by cgen, C23 profile. */
#include "app/app_mx_use.h"
#include "mx_box_2_3.h"
#include "mx_box_3_2.h"
#line 1 "app/mx_use.k"



int app_mx_use_main(void) {
    mx_box_2_3 a = {0};
    mx_box_2_3_set(&a, 0, 2, 7.0);
    mx_box_3_2 t = mx_box_2_3_transpose(&a);
    if (mx_box_3_2_get(&t, 2, 0) != 7.0) return 1;
    return 0;
}
