/* app/app_pos.type.h — generated from app/pos.k by cgen, C11 profile. */
#ifndef APP_APP_POS_TYPE_H
#define APP_APP_POS_TYPE_H
#include "keel.type.h"

#line 6 "app/pos.k"
#define app_pos_H ((size_t)2)
static const size_t app_pos_H__chk = 2;
#line 7 "app/pos.k"
#define app_pos_W ((size_t)3)
static const size_t app_pos_W__chk = 3;
#line 11 "app/pos.k"
struct app_pos_position {
    f32 *x;
    f32 *y;
    bool active;
    size_t len, cap;
};
#line 19 "app/pos.k"
struct app_pos_grid {
    i32 *v;
    size_t rows, cols, rcap, ccap;
};
#line 25 "app/pos.k"
struct app_pos_image {
    u8 r[app_pos_H][app_pos_W];
    u8 g[app_pos_H][app_pos_W];
    size_t h, w;
};
#endif /* APP_APP_POS_TYPE_H */
