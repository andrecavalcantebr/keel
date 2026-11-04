/* app/app_pbox_box_3_i32.h — generated from app/pbox.k by cgen, C11 profile. */
#ifndef APP_APP_PBOX_BOX_3_I32_H
#define APP_APP_PBOX_BOX_3_I32_H

#include "app/app_pbox_box_3_i32.type.h"

#line 7 "app/pbox.k"
static inline size_t app_pbox_box_3_i32_cap(app_pbox_box_3_i32 *b);
static inline app_pbox_box_3_i32_Pair app_pbox_box_3_i32_ends(app_pbox_box_3_i32 *b);

#line 7 "app/pbox.k"
static inline size_t app_pbox_box_3_i32_cap(app_pbox_box_3_i32 *b) { return app_pbox_box_3_i32_CAP; }
static inline app_pbox_box_3_i32_Pair app_pbox_box_3_i32_ends(app_pbox_box_3_i32 *b) { app_pbox_box_3_i32_Pair p = { b->v[0], b->v[3 - 1] }; return p; }

#endif /* APP_APP_PBOX_BOX_3_I32_H */
