/* app/app_val.c — generated from app/val.k by cgen, C23 profile. */
#include "app/app_val.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_slice_i32.h"
#include "keel/keel_parallel.h"
#line 1 "app/val.k"









bool app_val_has_negative(keel_buffer_i32 *xs) {
    keel_parallel_control validate = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, validate)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
#line 12 "app/val.k"
            { keel_slice_i32 keel__c1 = part; size_t keel__n1 = keel_slice_i32_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_slice_i32_get(keel__c1, i);
                if (x < 0) { atomic_fetch_add_explicit(&validate.fails, 1, memory_order_relaxed); goto keel__end0; }
            } }
            keel__end0: ;
        }
    }
#line 16 "app/val.k"
    return keel_parallel_failed(&validate);
}
