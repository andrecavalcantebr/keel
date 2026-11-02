/* app/app_pf.c — generated from app/pf.k by cgen, C11 profile. */
#include "app/app_pf.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_parallel.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/pf.k"







bool app_pf_two_lines(keel_buffer_i32 *xs) {
    keel_parallel_control v = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, v)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
#line 11 "app/pf.k"
            { keel_slice_i32 keel__c1 = part; size_t keel__n1 = keel_slice_i32_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_slice_i32_get(keel__c1, i);
                if (x < 0) { atomic_fetch_add_explicit(&v.fails, 1, memory_order_relaxed); goto keel__end0; }
            } }
            keel__end0: ;
        }
    }
#line 15 "app/pf.k"
    return keel_parallel_failed(&v);
}

bool app_pf_named(keel_buffer_i32 *xs) {
    keel_parallel_control v = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, v)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
#line 20 "app/pf.k"
            { keel_slice_i32 keel__c1 = part; size_t keel__n1 = keel_slice_i32_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_slice_i32_get(keel__c1, i);
                if (x < 0) { atomic_fetch_add_explicit(&v.fails, 1, memory_order_relaxed); goto keel__end0; }
            } }
            keel__end0: ;
        }
    }
#line 24 "app/pf.k"
    return keel_parallel_failed(&v);
}

bool app_pf_brace_below(keel_buffer_i32 *xs) {
    keel_parallel_control v = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, v)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
#line 30 "app/pf.k"
            { keel_slice_i32 keel__c1 = part; size_t keel__n1 = keel_slice_i32_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { i32 x = keel_slice_i32_get(keel__c1, i);
                if (x < 0) { atomic_fetch_add_explicit(&v.fails, 1, memory_order_relaxed); goto keel__end0; }
            } }
            keel__end0: ;
        }
    }
#line 34 "app/pf.k"
    return keel_parallel_failed(&v);
}
