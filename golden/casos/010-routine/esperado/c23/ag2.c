/* ag2.c — gerado de ag2.k pelo cgen, perfil C23. */
#include "ag2.h"
#include "keel/keel_corot.h"
#include "keel/keel_outcome_u32.h"
#include "keel/keel_routine_ag2_Ag.h"
#line 1 "ag2.k"








static keel_corot ag2_ola(ag2_Ag *g)       { keel_corot r = {0}; if (g->a-- > 0) return keel_corot_again(&r); return keel_corot_win(&r); }
static keel_corot ag2_autentica(ag2_Ag *g) { keel_corot r = {0}; if (g->b-- > 0) return keel_corot_again(&r); return keel_corot_win(&r); }
static keel_corot ag2_pronto(ag2_Ag *g)    { keel_corot r = {0}; if (g->c-- > 0) return keel_corot_again(&r); return keel_corot_win(&r); }
static keel_corot ag2_quebra(ag2_Ag *g)    { keel_corot r = {0}; return keel_corot_fault(&r, 9); }



keel_outcome_u32 ag2_cadeia(ag2_Ag *g) {
    keel_routine_slot_ag2_Ag passos[3] = {
        { .f = ag2_ola,       .ctx = g },
        { .f = ag2_autentica, .ctx = g },
        { .f = ag2_pronto,    .ctx = g },
    };
    return keel_routine_ag2_Ag_seq(keel_slice_keel_routine_slot_ag2_Ag_of(passos, 3));
}



keel_outcome_u32 ag2_cadeia_quebrada(ag2_Ag *g, i32 *estado_terceiro) {
    keel_routine_slot_ag2_Ag passos[3] = {
        { .f = ag2_ola,    .ctx = g },
        { .f = ag2_quebra, .ctx = g },
        { .f = ag2_pronto, .ctx = g },
    };
    keel_outcome_u32 r = keel_routine_ag2_Ag_seq(keel_slice_keel_routine_slot_ag2_Ag_of(passos, 3));
    *estado_terceiro = keel_routine_slot_ag2_Ag_code(&passos[2]);
    return r;
}



keel_outcome_u32 ag2_juntos(ag2_Ag *g) {
    keel_routine_slot_ag2_Ag passos[2] = {
        { .f = ag2_ola,       .ctx = g },
        { .f = ag2_autentica, .ctx = g },
    };
    return keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag_of(passos, 2), 1);
}


keel_outcome_u32 ag2_dois(ag2_Ag *g) {
    keel_routine_slot_ag2_Ag passos[3] = {
        { .f = ag2_quebra,    .ctx = g },
        { .f = ag2_ola,       .ctx = g },
        { .f = ag2_autentica, .ctx = g },
    };
    return keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag_of(passos, 3), 2);
}


keel_outcome_u32 ag2_impossivel(ag2_Ag *g) {
    keel_routine_slot_ag2_Ag passos[3] = {
        { .f = ag2_quebra, .ctx = g },
        { .f = ag2_quebra, .ctx = g },
        { .f = ag2_pronto, .ctx = g },
    };
    return keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag_of(passos, 3), 2);
}
