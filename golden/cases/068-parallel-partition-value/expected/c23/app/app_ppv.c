/* app/app_ppv.c — generated from app/ppv.k by cgen, C23 profile. */
#include "app/app_ppv.h"
#include "keel/keel_parallel.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/ppv.k"




bool app_ppv_all_non_negative(i32 *xs, size_t n) {
    keel_slice_i32 input = keel_slice_i32_from(xs, n);
    keel_parallel_control v = { .workers = 2, .target = 0 };
    {   keel_slice_i32 *keel__c0 = &input;

        for (size_t w = 0; w < 2; w++) {
            keel_slice_i32 part = keel_slice_i32_partition(*keel__c0, 2, w);
#line 8 "app/ppv.k"
            { keel_slice_i32 keel__c1 = part; size_t keel__n1 = keel_slice_i32_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_slice_i32_get(keel__c1, i); if (x < 0) { atomic_fetch_add_explicit(&v.fails, 1, memory_order_relaxed); goto keel__end0; } } }
            keel__end0: ;
        }
    }
#line 10 "app/ppv.k"
    return keel_parallel_ok(&v);
}
