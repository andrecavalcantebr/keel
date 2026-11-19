/* app/app_gfp_use.c — generated from app/gfp_use.k by cgen, C11 profile. */
#include "app/app_gfp_use.h"
#include "app/app_gfp.h"
#include "app/app_gfp_cell_7_u8.h"
#line 1 "app/gfp_use.k"



size_t app_gfp_use_outside(void) { return app_gfp_twice(21); }

size_t app_gfp_use_inside(void) {
    app_gfp_cell_7_u8 c = {0};
    return app_gfp_cell_7_u8_width2(&c);
}
