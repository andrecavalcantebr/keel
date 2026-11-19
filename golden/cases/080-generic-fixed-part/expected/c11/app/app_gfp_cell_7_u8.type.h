/* app/app_gfp_cell_7_u8.type.h — generated from app/gfp.k by cgen, C11 profile. */
#ifndef APP_APP_GFP_CELL_7_U8_TYPE_H
#define APP_APP_GFP_CELL_7_U8_TYPE_H

#include "keel.type.h"

#line 6 "app/gfp.k"
static_assert(7 >= 1 && 7 <= 8 * sizeof(u8), "gfp: W bits do not fit in a T");
#line 13 "app/gfp.k"
typedef struct app_gfp_cell_7_u8 { u8 v; } app_gfp_cell_7_u8;

#endif /* APP_APP_GFP_CELL_7_U8_TYPE_H */
