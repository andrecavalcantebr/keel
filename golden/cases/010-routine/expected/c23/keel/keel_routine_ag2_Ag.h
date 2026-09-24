/* keel/keel_routine_ag2_Ag.h — generated from keel/routine.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_ROUTINE_AG2_AG_H
#define KEEL_KEEL_ROUTINE_AG2_AG_H
#include "keel/keel_routine_ag2_Ag.type.h"
#include "keel/keel_outcome_u32.type.h"
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.type.h"

#line 27 "keel/routine.k"
static inline keel_corot keel_routine_slot_ag2_Ag_state(const keel_routine_slot_ag2_Ag *s);
#line 28 "keel/routine.k"
static inline i32        keel_routine_slot_ag2_Ag_code (const keel_routine_slot_ag2_Ag *s);

#line 33 "keel/routine.k"
static inline keel_outcome_u32 keel_routine_ag2_Ag_seq(keel_slice_keel_routine_slot_ag2_Ag s);

#line 52 "keel/routine.k"
static inline keel_outcome_u32 keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag s, u32 target);

#line 73 "keel/routine.k"
static inline u64 keel_routine_ag2_Ag_mask(keel_slice_keel_routine_slot_ag2_Ag s);
#include "keel/keel_corot.h"
#include "keel/keel_outcome_u32.h"
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.h"

#line 27 "keel/routine.k"
static inline keel_corot keel_routine_slot_ag2_Ag_state(const keel_routine_slot_ag2_Ag *s) { return s->state; }
#line 28 "keel/routine.k"
static inline i32        keel_routine_slot_ag2_Ag_code (const keel_routine_slot_ag2_Ag *s) { return s->state.code; }

#line 33 "keel/routine.k"
static inline keel_outcome_u32 keel_routine_ag2_Ag_seq(keel_slice_keel_routine_slot_ag2_Ag s) {
    keel_outcome_u32 r = {0};
    size_t n = keel_slice_keel_routine_slot_ag2_Ag_length(s);
    u32 S = 0;
    for (size_t i = 0; i < n; ++i) keel_slice_keel_routine_slot_ag2_Ag_ptr1(s, i)->state = (keel_corot){0};
    for (size_t i = 0; i < n; ++i) {
        keel_routine_slot_ag2_Ag *p = keel_slice_keel_routine_slot_ag2_Ag_ptr1(s, i);
        keel_corot c;
        do { c = p->f(p->ctx); } while (keel_corot_ongoing(c));
        p->state = c;
        if (keel_corot_faulted(c)) { keel_outcome_u32_value1(&r, S); return keel_outcome_u32_fail(&r, 1); }
        ++S;
    }
    return keel_outcome_u32_win1(&r, S);
}

#line 52 "keel/routine.k"
static inline keel_outcome_u32 keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag s, u32 target) {
    keel_outcome_u32 r = {0};
    size_t n = keel_slice_keel_routine_slot_ag2_Ag_length(s);
    KEEL_CHECK(target <= n, "par-target-above-total");
    u32 m = (u32)n, S = 0, F = 0, q = target ? target : m;
    for (size_t i = 0; i < n; ++i) keel_slice_keel_routine_slot_ag2_Ag_ptr1(s, i)->state = (keel_corot){0};
    for (;;) {
        for (size_t i = 0; i < n; ++i) {
            keel_routine_slot_ag2_Ag *p = keel_slice_keel_routine_slot_ag2_Ag_ptr1(s, i);
            if (!keel_corot_ongoing(p->state)) continue;
            p->state = p->f(p->ctx);
            if (keel_corot_ok(p->state))           ++S;
            else if (keel_corot_faulted(p->state)) ++F;
        }
        if (S >= q)    return keel_outcome_u32_win1(&r, S);
        if (m - F < q) { keel_outcome_u32_value1(&r, S); return keel_outcome_u32_fail(&r, 1); }
    }
}

#line 73 "keel/routine.k"
static inline u64 keel_routine_ag2_Ag_mask(keel_slice_keel_routine_slot_ag2_Ag s) {
    u64 m = 0;
    size_t n = keel_slice_keel_routine_slot_ag2_Ag_length(s);
    KEEL_CHECK(n <= 64, "mask-above-64-slots");
    for (size_t i = 0; i < n && i < 64; ++i)
        if (keel_corot_ok(keel_slice_keel_routine_slot_ag2_Ag_ptr1(s, i)->state)) m |= (u64)1 << i;
    return m;
}
#endif /* KEEL_KEEL_ROUTINE_AG2_AG_H */
