/* app/app_ofr.h — generated from app/ofr.k by cgen, C23 profile. */
#ifndef APP_APP_OFR_H
#define APP_APP_OFR_H
#include "app/app_ofr.type.h"
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_range.type.h"

#line 8 "app/ofr.k"
extern i32 app_ofr_v[6];
#line 13 "app/ofr.k"
keel_slice_i32 app_ofr_whole(void);
#line 14 "app/ofr.k"
keel_slice_i32 app_ofr_middle(size_t a, size_t b);
#line 15 "app/ofr.k"
keel_slice_i32 app_ofr_by_range(keel_range r);
#line 18 "app/ofr.k"
keel_slice_i32 app_ofr_same(keel_slice_i32 s);
#line 19 "app/ofr.k"
keel_slice_i32 app_ofr_sub(keel_slice_i32 s, keel_range r);
#line 20 "app/ofr.k"
keel_slice_i32 app_ofr_from_buffer(keel_range r);
#line 26 "app/ofr.k"
size_t app_ofr_count(void);
#line 27 "app/ofr.k"
i32 app_ofr_second(void);
#line 28 "app/ofr.k"
void app_ofr_put(size_t i, i32 x);
#line 29 "app/ofr.k"
i32 *app_ofr_at_ptr(size_t i);
#line 30 "app/ofr.k"
i32 app_ofr_checked(size_t i);
#line 36 "app/ofr.k"
size_t app_ofr_inverted(void);
#line 37 "app/ofr.k"
size_t app_ofr_literal(void);
#endif /* APP_APP_OFR_H */
