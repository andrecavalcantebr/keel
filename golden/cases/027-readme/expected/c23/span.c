/* span.c — generated from span.k by cgen, C23 profile. */
#include "span.h"
#include "keel/keel_range.h"
#include "keel/keel_slice_i16.h"
#include "keel/keel_slice_of_keel_array_i16.h"
#line 1 "span.k"





i16 span_sum(void) {
    i16 dados[3] = {1, 2, 3};
    keel_slice_i16 part = keel_slice_of2_keel_array_i16(dados, 3, 1, 3);
    *keel_slice_i16_ptr1(part, 0) = 20;
    *(keel_slice_i16_ptr(part) + 1) = 30;

    keel_range indices = keel_range_of(0, 2);
    i16 total = 0;
    { size_t keel__f0 = keel_range_first(indices); size_t keel__l0 = keel_range_limit(indices); for (size_t i = keel__f0; i < keel__l0; i++) {
        total += *keel_slice_i16_ptr1(part, i);
    } }
    return total;
}
