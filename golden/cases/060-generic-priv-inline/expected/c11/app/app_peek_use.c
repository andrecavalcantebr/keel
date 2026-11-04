/* app/app_peek_use.c — generated from app/peek_use.k by cgen, C11 profile. */
#include "app/app_peek_use.h"
#include "app/app_peek_box_i32.h"
#include "app/app_peek_box_i64.h"
#line 1 "app/peek_use.k"



i32 app_peek_use_read_i32(i32 v) {
    app_peek_box_i32 b;
    b.v = v;
    return app_peek_box_i32_get(&b);
}

i64 app_peek_use_read_i64(i64 v) {
    app_peek_box_i64 w;
    w.v = v;
    return app_peek_box_i64_get(&w);
}
