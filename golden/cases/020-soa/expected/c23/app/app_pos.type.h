/* app/app_pos.type.h — generated from app/pos.k by cgen, C23 profile. */
#ifndef APP_APP_POS_TYPE_H
#define APP_APP_POS_TYPE_H
#include "keel.type.h"

#line 12 "app/pos.k"
typedef struct app_pos_position {
    f32 *x;
    f32 *y;
    bool active;
    size_t len, cap;
} app_pos_position;
#endif /* APP_APP_POS_TYPE_H */
