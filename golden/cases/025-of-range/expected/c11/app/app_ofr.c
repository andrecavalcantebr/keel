/* app/app_ofr.c — generated from app/ofr.k by cgen, C11 profile. */
#include "app/app_ofr.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_outcome_i32.h"
#include "keel/keel_range.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/ofr.k"







i32 app_ofr_v[6] = {10, 11, 12, 13, 14, 15};




keel_slice_i32 app_ofr_whole(void) { return keel_slice_i32_from(app_ofr_v, 6); }
keel_slice_i32 app_ofr_middle(size_t a, size_t b) { return keel_slice_i32_of2(keel_slice_i32_from(app_ofr_v, 6), a, b); }
keel_slice_i32 app_ofr_by_range(keel_range r) { return keel_slice_i32_of1(keel_slice_i32_from(app_ofr_v, 6), r); }


keel_slice_i32 app_ofr_same(keel_slice_i32 s) { return keel_slice_i32_of(s); }
keel_slice_i32 app_ofr_sub(keel_slice_i32 s, keel_range r) { return keel_slice_i32_of1(s, r); }
keel_slice_i32 app_ofr_from_buffer(keel_range r) {
    keel_buffer_i32 b = keel_buffer_i32_of(app_ofr_v, 6);
    return keel_buffer_i32_as_slice1(&b, r);
}


size_t app_ofr_count(void) { return sizeof(app_ofr_v)/sizeof(i32); }
i32 app_ofr_second(void) { return app_ofr_v[1]; }
void app_ofr_put(size_t i, i32 x) { app_ofr_v[keel_index(i, 6)] = x; }
i32 *app_ofr_at_ptr(size_t i) { return &app_ofr_v[keel_index(i, 6)]; }
i32 app_ofr_checked(size_t i) {
    keel_outcome_i32 r = keel_slice_i32_at(keel_slice_i32_from(app_ofr_v, 6), i); if (keel_outcome_i32_failed(r)) return -1;
    return keel_outcome_i32_value(r);
}


size_t app_ofr_inverted(void) { keel_range r = keel_range_of(5, 2); return keel_range_length(r); }
size_t app_ofr_literal(void) { keel_range r = keel_range_of(2, 5); return keel_range_length(r); }
