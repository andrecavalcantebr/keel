/* app/app_dbg.c — generated from app/dbg.k by cgen, C11 profile. */
#include "app/app_dbg.h"
#include "keel/keel_corot.h"
#include "keel/keel_outcome_u32.h"
#include "keel/keel_routine_i16.h"
#include "keel/keel_slice_of_keel_array_keel_routine_slot_i16.h"
#line 1 "app/dbg.k"

















static keel_corot app_dbg_stage(i16 *counter) {
    keel_corot r = {0};
    ++*counter;
    return keel_corot_win(&r);
}



int app_dbg_main(int argc, char **argv) {
    int which = argc > 1 ? atoi(argv[1]) : 0;
    i16 c = 0;
    if (which == 1) {
        struct app_dbg_image img = {0};
        img.h = 1;
        img.w = 1;
        return *app_dbg_image_r_ptr(&img, 1, 0);
    }
    if (which == 2) {
        keel_routine_slot_i16 steps[2] = {
            { .f = app_dbg_stage, .ctx = &c, .state = {0} },
            { .f = app_dbg_stage, .ctx = &c, .state = {0} },
        };
        keel_outcome_u32 r = keel_routine_i16_par(keel_slice_of_keel_array_keel_routine_slot_i16(steps, 2), 3);
        return keel_outcome_u32_ok(r) ? 0 : 1;
    }
    if (which == 3) {
        keel_routine_slot_i16 many[65];
        return (int)keel_routine_i16_mask(keel_slice_of_keel_array_keel_routine_slot_i16(many, 65));
    }
    return 0;
}
