/* keel/keel_routine_ag2_Ag.type.h — generated from keel/routine.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H
#define KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H
#include "keel.type.h"
#include "keel/keel_corot.type.h"
#include "ag2.type.h"

#line 18 "keel/routine.k"
typedef keel_corot (*keel_routine_ag2_Ag)(ag2_Ag *);
#line 25 "keel/routine.k"
typedef struct keel_routine_slot_ag2_Ag {
    keel_routine_ag2_Ag  f;
    ag2_Ag              *ctx;
    keel_corot           state;
} keel_routine_slot_ag2_Ag;
#endif /* KEEL_KEEL_ROUTINE_AG2_AG_TYPE_H */
