/* coop.c — generated from coop.k by cgen, C23 profile. */
#include "coop.h"
#include "keel/keel_corot.h"
#include "keel/keel_outcome_u32.h"
#include "keel/keel_routine_i16.h"
#include "keel/keel_slice_of_keel_array_keel_routine_slot_i16.h"
#include "keel/keel_tagged_coop_Cycle_void.h"
#line 1 "coop.k"





static keel_corot coop_count(i16 *counter, i16 limit, bool failure) {
    keel_corot r = {0};
    ++*counter;
    if (*counter < limit) return keel_corot_again(&r);
    if (failure)              return keel_corot_fault(&r, 1);
    return keel_corot_win(&r);
}

static void coop_note(i16 *counter) {
    ++*counter;
}



i16 coop_manual(void) {
    keel_tagged_coop_Cycle_void state = {0};
    keel_tagged_coop_Cycle_void_mark(&state, coop_Cycle_WAIT);
    i16 calls = 0;
    bool finished = false;

    while (!finished) {
        switch (state.tag) {
        case coop_Cycle_WAIT: goto keel__m0_WAIT;
        case coop_Cycle_END:  goto keel__m0_END;
        default:              KEEL_CHECK(0, "tag-out-of-range"); goto keel__m0_end;
        }
#line 28 "coop.k"
        keel__m0_WAIT: {
            keel_corot r = coop_count(&calls, 2, false);
            if (keel_corot_faulted(r) || keel_corot_ok(r)) keel_tagged_coop_Cycle_void_mark(&state, coop_Cycle_END);
        }
        goto keel__m0_end;
#line 31 "coop.k"
        keel__m0_END: {
            finished = true;
        }
        keel__m0_end: ;
#line 34 "coop.k"
    }
    return calls;
}





static keel_corot coop_stage(i16 *counter) {
    keel_corot r = {0};
    ++*counter;
    if (*counter < 2) return keel_corot_again(&r);
    return keel_corot_win(&r);
}

keel_outcome_u32 coop_set_of(i16 *a, i16 *b) {
    keel_routine_slot_i16 steps[2] = {
        { .f = coop_stage, .ctx = a, .state = {0} },
        { .f = coop_stage, .ctx = b, .state = {0} },
    };

    keel_outcome_u32 r = keel_routine_i16_par(keel_slice_of_keel_array_keel_routine_slot_i16(steps, 2), 1);

    { for (size_t i = 0; i < 2; i++) { keel_routine_slot_i16 *sl = &steps[i];
        if (keel_corot_ok(keel_routine_slot_i16_state(sl))) coop_note(a);
    } }
    return r;
}
