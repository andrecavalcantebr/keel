/* results.c — generated from results.k by cgen, C11 profile. */
#include "results.h"
#include "keel/keel_corot.h"
#include "keel/keel_outcome_i32.h"
#include "keel/keel_outcome_void.h"
#line 1 "results.k"




keel_corot results_state(i32 code) {
    keel_corot r = {0};
    if (code > 0) return keel_corot_fault(&r, code);
    if (code == 0) return keel_corot_again(&r);
    return keel_corot_win(&r);
}

bool results_is_fault(i32 code) {
    keel_corot r = results_state(code);
    return keel_corot_faulted(r);
}

keel_outcome_void results_final(bool failure, i32 code) {
    keel_outcome_void r = {0};
    if (failure) return keel_outcome_void_fail(&r, code);
    return keel_outcome_void_win(&r);
}

i32 results_repair(i32 code) {
    keel_outcome_i32 origin = {0};
    keel_outcome_i32_fail(&origin, code);
    keel_outcome_i32 r = origin; if (keel_outcome_i32_failed(r)) keel_outcome_i32_win1(&r, 5);
    return keel_outcome_i32_value(r);
}



keel_corot results_evaluate_cleanup(i32 *counter) {
    keel_corot r = {0};
    keel_corot keel__rv0;
    keel__rv0 = keel_corot_fault(&r, ++*counter); ++*counter; return keel__rv0;
}
