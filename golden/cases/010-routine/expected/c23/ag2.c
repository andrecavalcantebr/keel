/* ag2.c — generated from ag2.k by cgen, C23 profile. */
#include "ag2.h"
#include "keel/keel_corot.h"
#include "keel/keel_outcome_u32.h"
#include "keel/keel_routine_ag2_Ag.h"
#line 1 "ag2.k"








static keel_corot ag2_hello(ag2_Ag *g)       { keel_corot r = {0}; if (g->a-- > 0) return keel_corot_again(&r); return keel_corot_win(&r); }
static keel_corot ag2_authenticate(ag2_Ag *g) { keel_corot r = {0}; if (g->b-- > 0) return keel_corot_again(&r); return keel_corot_win(&r); }
static keel_corot ag2_ready(ag2_Ag *g)    { keel_corot r = {0}; if (g->c-- > 0) return keel_corot_again(&r); return keel_corot_win(&r); }
static keel_corot ag2_faults(ag2_Ag *g)    { keel_corot r = {0}; return keel_corot_fault(&r, 9); }



keel_outcome_u32 ag2_chain(ag2_Ag *g) {
    keel_routine_slot_ag2_Ag steps[3] = {
        { .f = ag2_hello,       .ctx = g },
        { .f = ag2_authenticate, .ctx = g },
        { .f = ag2_ready,    .ctx = g },
    };
    return keel_routine_ag2_Ag_seq(keel_slice_keel_routine_slot_ag2_Ag_of(steps, 3));
}



keel_outcome_u32 ag2_broken_chain(ag2_Ag *g, i32 *third_state) {
    keel_routine_slot_ag2_Ag steps[3] = {
        { .f = ag2_hello,    .ctx = g },
        { .f = ag2_faults, .ctx = g },
        { .f = ag2_ready, .ctx = g },
    };
    keel_outcome_u32 r = keel_routine_ag2_Ag_seq(keel_slice_keel_routine_slot_ag2_Ag_of(steps, 3));
    *third_state = keel_routine_slot_ag2_Ag_code(&steps[2]);
    return r;
}



keel_outcome_u32 ag2_together(ag2_Ag *g) {
    keel_routine_slot_ag2_Ag steps[2] = {
        { .f = ag2_hello,       .ctx = g },
        { .f = ag2_authenticate, .ctx = g },
    };
    return keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag_of(steps, 2), 1);
}


keel_outcome_u32 ag2_two(ag2_Ag *g) {
    keel_routine_slot_ag2_Ag steps[3] = {
        { .f = ag2_faults,    .ctx = g },
        { .f = ag2_hello,       .ctx = g },
        { .f = ag2_authenticate, .ctx = g },
    };
    return keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag_of(steps, 3), 2);
}


keel_outcome_u32 ag2_impossible(ag2_Ag *g) {
    keel_routine_slot_ag2_Ag steps[3] = {
        { .f = ag2_faults, .ctx = g },
        { .f = ag2_faults, .ctx = g },
        { .f = ag2_ready, .ctx = g },
    };
    return keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag_of(steps, 3), 2);
}
