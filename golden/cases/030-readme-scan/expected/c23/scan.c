/* scan.c — generated from scan.k by cgen, C23 profile. */
#include "scan.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_buffer_u32.h"
#include "keel/keel_parallel.h"
#include "keel/keel_slice.h"
#include "keel/keel_slice_i32.h"
#line 1 "scan.k"





void scan_sum(keel_buffer_i32 *xs, keel_buffer_u32 *totals) {
    keel_parallel_control sum = { .workers = 4, .target = 0 };
    {   keel_buffer_i32 *keel__c0 = xs;

        #pragma omp parallel for num_threads(4) default(none) \
                shared(keel__c0, sum, totals)
        for (size_t w = 0; w < 4; w++) {
            keel_slice_i32 part = keel_buffer_i32_partition(keel__c0, 4, w);
#line 8 "scan.k"
            u32 t = 0;
            {   keel_slice_i32 keel__c1 = part; keel_slice_cursor c = keel_slice_i32_begin(keel__c1); while (keel_slice_i32_has_next(keel__c1, &c)) { i32 *p = keel_slice_i32_next(keel__c1, &c);
                t += (u32)*p;
            } }
            *keel_buffer_u32_ptr1(totals, w) = t;
            keel__end0: ;
        }
    }
#line 14 "scan.k"
}
