/* app/app_dbg.type.h — generated from app/dbg.k by cgen, C23 profile. */
#ifndef APP_APP_DBG_TYPE_H
#define APP_APP_DBG_TYPE_H

#include "keel.type.h"

#line 7 "app/dbg.k"
#include <stdlib.h>
#line 9 "app/dbg.k"
constexpr size_t app_dbg_H = 2;
constexpr size_t app_dbg_W = 3;
#line 13 "app/dbg.k"
struct app_dbg_image {
    u8 r[app_dbg_H][app_dbg_W];
    size_t h, w;
};

#endif /* APP_APP_DBG_TYPE_H */
