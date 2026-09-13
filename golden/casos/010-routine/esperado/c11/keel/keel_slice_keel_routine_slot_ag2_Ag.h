/* keel/keel_slice_keel_routine_slot_ag2_Ag.h — instância de `slice slot`.
   Arrastada pela instanciação de `keel.routine` (backend §4.3). */
#ifndef KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#define KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#include "keel/prelude.h"
#include "keel/slice.h"

typedef struct keel_slice_keel_routine_slot_ag2_Ag {
    size_t len;
    keel_routine_slot_ag2_Ag *ptr;
} keel_slice_keel_routine_slot_ag2_Ag;

static inline keel_slice_keel_routine_slot_ag2_Ag
keel_slice_keel_routine_slot_ag2_Ag_of(keel_routine_slot_ag2_Ag *p, size_t n) {
    return (keel_slice_keel_routine_slot_ag2_Ag){ p ? n : 0, p };
}
static inline size_t keel_slice_keel_routine_slot_ag2_Ag_length(keel_slice_keel_routine_slot_ag2_Ag s) { return s.len; }
static inline keel_routine_slot_ag2_Ag *
keel_slice_keel_routine_slot_ag2_Ag_ptr1(keel_slice_keel_routine_slot_ag2_Ag s, size_t i) { return &s.ptr[i]; }
#endif
