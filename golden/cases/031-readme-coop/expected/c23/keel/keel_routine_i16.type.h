/* keel/keel_routine_i16.type.h — generated from keel/routine.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_ROUTINE_I16_TYPE_H
#define KEEL_KEEL_ROUTINE_I16_TYPE_H

#include "keel.type.h"
#include "keel/keel_corot.type.h"

#line 18 "keel/routine.k"
typedef keel_corot (*keel_routine_i16)(i16 *ctx);
#line 25 "keel/routine.k"
typedef struct keel_routine_slot_i16 { keel_routine_i16 f; i16 *ctx; keel_corot state; } keel_routine_slot_i16;

#endif /* KEEL_KEEL_ROUTINE_I16_TYPE_H */
