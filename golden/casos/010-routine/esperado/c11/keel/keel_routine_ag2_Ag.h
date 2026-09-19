/* keel/keel_routine_ag2_Ag.h — gerado de keel/routine.k pelo cgen, perfil C11. */
#ifndef KEEL_KEEL_ROUTINE_AG2_AG_H
#define KEEL_KEEL_ROUTINE_AG2_AG_H
#include "keel/keel_routine_ag2_Ag.type.h"
#include "keel/keel_outcome_u32.type.h"
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.type.h"

static inline keel_corot keel_routine_slot_ag2_Ag_state(const keel_routine_slot_ag2_Ag *s);
static inline i32        keel_routine_slot_ag2_Ag_code (const keel_routine_slot_ag2_Ag *s);
/* `seq`: a etapa corrente é chamada até deixar de ceder; SUCCESS avança,
   FAILED encerra sem chamar as seguintes (linguagem §5.6). */
static inline keel_outcome_u32 keel_routine_ag2_Ag_seq(keel_slice_keel_routine_slot_ag2_Ag s);
/* `par`: um ciclo chama uma vez cada slot ainda em ONGOING; a política é
   avaliada ao fim do ciclo. Alvo zero significa todos (linguagem §5.6). */
static inline keel_outcome_u32 keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag s, u32 target);
/* conveniência derivada da tabela, não a representação do resultado */
static inline u64 keel_routine_ag2_Ag_mask(keel_slice_keel_routine_slot_ag2_Ag s);
#include "keel/keel_corot.h"
#include "keel/keel_outcome_u32.h"
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.h"

static inline keel_corot keel_routine_slot_ag2_Ag_state(const keel_routine_slot_ag2_Ag *s) { return s->state; }
static inline i32        keel_routine_slot_ag2_Ag_code (const keel_routine_slot_ag2_Ag *s) { return s->state.code; }
/* `seq`: a etapa corrente é chamada até deixar de ceder; SUCCESS avança,
   FAILED encerra sem chamar as seguintes (linguagem §5.6). */
static inline keel_outcome_u32 keel_routine_ag2_Ag_seq(keel_slice_keel_routine_slot_ag2_Ag s) {
    keel_outcome_u32 r = {0};
    u32 S = 0;
    for (size_t i = 0; i < s.len; ++i) s.ptr[i].state = (keel_corot){0};
    for (size_t i = 0; i < s.len; ++i) {
        keel_corot c;
        do { c = s.ptr[i].f(s.ptr[i].ctx); } while (keel_corot_ongoing(c));
        s.ptr[i].state = c;
        if (keel_corot_faulted(c)) { keel_outcome_u32_value1(&r, S); return keel_outcome_u32_fail(&r, 1); }
        ++S;
    }
    return keel_outcome_u32_win1(&r, S);
}
/* `par`: um ciclo chama uma vez cada slot ainda em ONGOING; a política é
   avaliada ao fim do ciclo. Alvo zero significa todos (linguagem §5.6). */
static inline keel_outcome_u32 keel_routine_ag2_Ag_par(keel_slice_keel_routine_slot_ag2_Ag s, u32 target) {
    keel_outcome_u32 r = {0};
    u32 m = (u32)s.len, S = 0, F = 0, q = target ? target : m;
    for (size_t i = 0; i < s.len; ++i) s.ptr[i].state = (keel_corot){0};
    for (;;) {
        for (size_t i = 0; i < s.len; ++i) {
            if (!keel_corot_ongoing(s.ptr[i].state)) continue;
            s.ptr[i].state = s.ptr[i].f(s.ptr[i].ctx);
            if (keel_corot_ok(s.ptr[i].state))           ++S;
            else if (keel_corot_faulted(s.ptr[i].state)) ++F;
        }
        if (S >= q)    return keel_outcome_u32_win1(&r, S);
        if (m - F < q) { keel_outcome_u32_value1(&r, S); return keel_outcome_u32_fail(&r, 1); }
    }
}
/* conveniência derivada da tabela, não a representação do resultado */
static inline u64 keel_routine_ag2_Ag_mask(keel_slice_keel_routine_slot_ag2_Ag s) {
    u64 m = 0;
    for (size_t i = 0; i < s.len && i < 64; ++i)
        if (keel_corot_ok(s.ptr[i].state)) m |= (u64)1 << i;
    return m;
}
#endif /* KEEL_KEEL_ROUTINE_AG2_AG_H */
