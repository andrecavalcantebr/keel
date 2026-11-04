/* app/app_peek_box_i64.h — generated from app/peek.k by cgen, C11 profile. */
#ifndef APP_APP_PEEK_BOX_I64_H
#define APP_APP_PEEK_BOX_I64_H

#include "app/app_peek_box_i64.type.h"

#line 3 "app/peek.k"
static inline i64 app_peek_box_i64_peek(app_peek_box_i64 *b);
static inline i64 app_peek_box_i64_get(app_peek_box_i64 *b);

#line 3 "app/peek.k"
static inline i64 app_peek_box_i64_peek(app_peek_box_i64 *b) { return b->v; }
static inline i64 app_peek_box_i64_get(app_peek_box_i64 *b) { return app_peek_box_i64_peek(b); }

#endif /* APP_APP_PEEK_BOX_I64_H */
