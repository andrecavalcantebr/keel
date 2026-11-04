/* app/app_pbox_use.c — generated from app/pbox_use.k by cgen, C11 profile. */
#include "app/app_pbox_use.h"
#include "app/app_pbox_box_3_i32.h"
#line 1 "app/pbox_use.k"



size_t app_pbox_use_probe_cap(void) { app_pbox_box_3_i32 b; return app_pbox_box_3_i32_cap(&b); }
i32 app_pbox_use_probe_first(void) { app_pbox_box_3_i32 b; b.v[0] = 4; b.v[1] = 5; b.v[2] = 6; return app_pbox_box_3_i32_ends(&b).first; }
i32 app_pbox_use_probe_last(void) { app_pbox_box_3_i32 b; b.v[0] = 4; b.v[1] = 5; b.v[2] = 6; return app_pbox_box_3_i32_ends(&b).last; }
