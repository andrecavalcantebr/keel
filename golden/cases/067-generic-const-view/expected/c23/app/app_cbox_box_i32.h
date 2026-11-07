/* app/app_cbox_box_i32.h — generated from app/cbox.k by cgen, C23 profile. */
#ifndef APP_APP_CBOX_BOX_I32_H
#define APP_APP_CBOX_BOX_I32_H

#include "app/app_cbox_box_i32.type.h"
#include "keel/keel_slice_const_i32.type.h"

#line 6 "app/cbox.k"
static inline keel_slice_const_i32 app_cbox_box_i32_view(app_cbox_box_i32 *b);

#line 6 "app/cbox.k"
static inline keel_slice_const_i32 app_cbox_box_i32_view(app_cbox_box_i32 *b) {
    keel_slice_const_i32 s = { b->length, b->values };
    return s;
}

#endif /* APP_APP_CBOX_BOX_I32_H */
