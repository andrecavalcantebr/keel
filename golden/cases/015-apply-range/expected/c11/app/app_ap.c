/* app/app_ap.c — generated from app/ap.k by cgen, C11 profile. */
#include "app/app_ap.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_range.h"
#line 1 "app/ap.k"




static i32 app_ap_acc = 0;
static void app_ap_doubler (i32 v, size_t i)        { app_ap_acc += v * 2 + (i32)i; }
static void app_ap_scale(i32 v, size_t i, i32 k) { app_ap_acc += v * k + (i32)i; }
i32  app_ap_sum_acc(void) { return app_ap_acc; }




void app_ap_traverse(keel_buffer_i32 *xs) {
    { keel_buffer_i32 *keel__c0 = xs; size_t keel__n0 = keel_buffer_i32_length(keel__c0); for (size_t keel__i0 = 0; keel__i0 < keel__n0; keel__i0++) { i32 keel__v0 = keel_buffer_i32_get(keel__c0, keel__i0); app_ap_doubler(keel__v0, keel__i0); } }
    { keel_buffer_i32 *keel__c1 = xs; size_t keel__n1 = keel_buffer_i32_length(keel__c1); for (size_t keel__i1 = 0; keel__i1 < keel__n1; keel__i1++) { i32 keel__v1 = keel_buffer_i32_get(keel__c1, keel__i1); app_ap_scale(keel__v1, keel__i1, 3); } }
}



size_t app_ap_sum_range(keel_range r) {
    size_t t = 0;
    { size_t keel__f2 = keel_range_first(r); size_t keel__l2 = keel_range_limit(r); for (size_t v = keel__f2; v < keel__l2; v++) { t += v; } }
    { keel_range keel__c3 = r; size_t keel__n3 = keel_range_length(keel__c3); for (size_t i = 0; i < keel__n3; i++) { size_t v = keel_range_get(keel__c3, i); t += v * i; } }
    return t;
}
