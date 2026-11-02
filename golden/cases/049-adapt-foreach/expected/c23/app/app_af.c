/* app/app_af.c — generated from app/af.k by cgen, C23 profile. */
#include "app/app_af.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_range.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/af.k"





static keel_buffer_i32 *app_af_pick(keel_buffer_i32 *a) { return a; }

size_t app_af_adapt(keel_slice_i32 *sp, keel_range *r) {
    return keel_slice_i32_length(*sp) + keel_range_first(*r) + keel_range_limit(*r);
}

size_t app_af_over_range(keel_range *r) {
    size_t t = 0;
    { size_t keel__f0 = keel_range_first(*r); size_t keel__l0 = keel_range_limit(*r); for (size_t v = keel__f0; v < keel__l0; v++) t += v; }
    return t;
}

int app_af_over_call(keel_buffer_i32 *xs) {
    int t = 0;
    { keel_buffer_i32 *keel__c0 = app_af_pick(xs); size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { i32 x = keel_buffer_i32_get(keel__c0, i); t += x; } }
    return t;
}
