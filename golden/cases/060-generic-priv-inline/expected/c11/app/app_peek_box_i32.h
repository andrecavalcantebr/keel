/* app/app_peek_box_i32.h — generated from app/peek.k by cgen, C11 profile. */
#ifndef APP_APP_PEEK_BOX_I32_H
#define APP_APP_PEEK_BOX_I32_H

#include "app/app_peek_box_i32.type.h"

#line 3 "app/peek.k"
static inline i32 app_peek_box_i32_peek(app_peek_box_i32 *b);
static inline i32 app_peek_box_i32_get(app_peek_box_i32 *b);

#line 3 "app/peek.k"
static inline i32 app_peek_box_i32_peek(app_peek_box_i32 *b) { return b->v; }
static inline i32 app_peek_box_i32_get(app_peek_box_i32 *b) { return app_peek_box_i32_peek(b); }

#endif /* APP_APP_PEEK_BOX_I32_H */
