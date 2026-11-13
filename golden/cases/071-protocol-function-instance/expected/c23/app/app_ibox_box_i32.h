/* app/app_ibox_box_i32.h — generated from app/ibox.k by cgen, C23 profile. */
#ifndef APP_APP_IBOX_BOX_I32_H
#define APP_APP_IBOX_BOX_I32_H

#include "app/app_ibox_box_i32.type.h"

#line 5 "app/ibox.k"
static inline size_t app_ibox_box_i32_length(app_ibox_box_i32 *b);
[[nodiscard]] static inline i32 *app_ibox_box_i32_ptr(app_ibox_box_i32 *b, size_t i);

#line 5 "app/ibox.k"
static inline size_t app_ibox_box_i32_length(app_ibox_box_i32 *b) { return b->length; }
[[nodiscard]] static inline i32 *app_ibox_box_i32_ptr(app_ibox_box_i32 *b, size_t i) { return &b->values[i]; }

#endif /* APP_APP_IBOX_BOX_I32_H */
