/* keel/keel_routine_ag2_Ag.type.h — instância de `keel.routine` sobre o contexto
   `ag2.Ag` (backend §5.10). O typedef da participante é da instância; o estado
   por slot é um `corot`, e não um inteiro nu. */
#ifndef KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H
#define KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H
#include "keel/prelude.h"
#include "keel/corot.type.h"
#include "ag2/ag2.type.h"

typedef keel_corot (*keel_routine_ag2_Ag)(ag2_Ag *);
typedef struct keel_routine_slot_ag2_Ag {
    keel_routine_ag2_Ag  f;
    ag2_Ag              *ctx;
    keel_corot           state;
} keel_routine_slot_ag2_Ag;
#endif /* KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H */
