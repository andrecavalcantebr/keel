/* app/app_dbg.h — generated from app/dbg.k by cgen, C23 profile. */
#ifndef APP_APP_DBG_H
#define APP_APP_DBG_H

#include "app/app_dbg.type.h"

#line 14 "app/dbg.k"
static inline u8 *app_dbg_image_r_ptr(struct app_dbg_image *p, size_t i0, size_t i1);
#line 26 "app/dbg.k"
int app_dbg_main(int argc, char **argv);

#line 14 "app/dbg.k"
static inline u8 *app_dbg_image_r_ptr(struct app_dbg_image *p, size_t i0, size_t i1) {
#line 14 "app/dbg.k"
    KEEL_CHECK(i0 < p->h && p->h <= app_dbg_H, "extent-index-out-of-bounds");
#line 14 "app/dbg.k"
    KEEL_CHECK(i1 < p->w && p->w <= app_dbg_W, "extent-index-out-of-bounds");
    return &p->r[i0][i1];
}

#endif /* APP_APP_DBG_H */
