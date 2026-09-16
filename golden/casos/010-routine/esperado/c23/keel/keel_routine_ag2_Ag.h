/* keel/keel_routine_ag2_Ag.h — instância de `keel.routine` sobre o contexto
   `ag2.Ag` (backend §5.10). O typedef da participante é da instância; o estado
   por slot é um `corot`, e não um inteiro nu. */
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
#endif /* KEEL_KEEL_ROUTINE_AG2_AG_H */
