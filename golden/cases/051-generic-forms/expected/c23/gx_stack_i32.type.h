/* gx_stack_i32.type.h — generated from gx.k by cgen, C23 profile. */
#ifndef GX_STACK_I32_TYPE_H
#define GX_STACK_I32_TYPE_H

#include "keel.type.h"

#line 3 "gx.k"
typedef struct gx_stack_i32 {
    size_t cap, len;
    i32 *ptr;
} gx_stack_i32;

#endif /* GX_STACK_I32_TYPE_H */
