/* keel/keel_routine_ag2_Ag.type.h — gerado de keel/routine.k pelo cgen, perfil C11. */
#ifndef KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H
#define KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H
#include "keel.type.h"
#include "keel/keel_corot.type.h"
#include "ag2.type.h"

typedef keel_corot (*keel_routine_ag2_Ag)(ag2_Ag *);
typedef struct keel_routine_slot_ag2_Ag {
    keel_routine_ag2_Ag  f;
    ag2_Ag              *ctx;
    keel_corot           state;
} keel_routine_slot_ag2_Ag;
#endif /* KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H */
