/* app/app_dbg.type.h — generated from app/dbg.k by cgen, C11 profile. */
#ifndef APP_APP_DBG_TYPE_H
#define APP_APP_DBG_TYPE_H

#include "keel.type.h"

#line 7 "app/dbg.k"
#include <stdlib.h>
#line 9 "app/dbg.k"
#define app_dbg_H ((size_t)2)
#line 9 "app/dbg.k"
static const size_t app_dbg_H__chk = 2; _Static_assert(sizeof(app_dbg_H__chk) > 0, "constexpr");
#define app_dbg_W ((size_t)3)
#line 10 "app/dbg.k"
static const size_t app_dbg_W__chk = 3; _Static_assert(sizeof(app_dbg_W__chk) > 0, "constexpr");
#line 13 "app/dbg.k"
struct app_dbg_image {
    u8 r[app_dbg_H][app_dbg_W];
    size_t h, w;
};

#endif /* APP_APP_DBG_TYPE_H */
