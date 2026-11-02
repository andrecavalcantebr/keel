/* app/app_af.h — generated from app/af.k by cgen, C11 profile. */
#ifndef APP_APP_AF_H
#define APP_APP_AF_H

#include "app/app_af.type.h"

typedef struct keel_slice_i32 keel_slice_i32;
typedef struct keel_range keel_range;
typedef struct keel_buffer_i32 keel_buffer_i32;
#line 8 "app/af.k"
size_t app_af_adapt(keel_slice_i32 *sp, keel_range *r);
#line 12 "app/af.k"
size_t app_af_over_range(keel_range *r);
#line 18 "app/af.k"
int app_af_over_call(keel_buffer_i32 *xs);

#endif /* APP_APP_AF_H */
