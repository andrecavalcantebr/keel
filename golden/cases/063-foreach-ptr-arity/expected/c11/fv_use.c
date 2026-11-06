/* fv_use.c — generated from fv_use.k by cgen, C11 profile. */
#include "fv_use.h"
#include "fv_vec_i32.h"
#line 1 "fv_use.k"



i32 fv_use_sum3(i32 a, i32 b, i32 c) {
    i32 buf[3] = { a, b, c };
    fv_vec_i32 v = { 3, buf };
    i32 total = 0;
    { fv_vec_i32 *keel__c0 = &v; size_t keel__n0 = fv_vec_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { i32 *p = fv_vec_i32_ptr(keel__c0, i); total += *p; } }
    return total;
}
