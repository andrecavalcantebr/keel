/* keel/keel_slice_keel_routine_slot_ag2_Ag.h — gerado de keel/slice.k pelo cgen, perfil C11. */
#ifndef KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#define KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_slice.type.h"

static inline keel_slice_keel_routine_slot_ag2_Ag
keel_slice_keel_routine_slot_ag2_Ag_of(keel_routine_slot_ag2_Ag *p, size_t n);
static inline size_t keel_slice_keel_routine_slot_ag2_Ag_length(keel_slice_keel_routine_slot_ag2_Ag s);
static inline keel_routine_slot_ag2_Ag *
keel_slice_keel_routine_slot_ag2_Ag_ptr1(keel_slice_keel_routine_slot_ag2_Ag s, size_t i);

static inline keel_slice_keel_routine_slot_ag2_Ag
keel_slice_keel_routine_slot_ag2_Ag_of(keel_routine_slot_ag2_Ag *p, size_t n) {
    return (keel_slice_keel_routine_slot_ag2_Ag){ p ? n : 0, p };
}
static inline size_t keel_slice_keel_routine_slot_ag2_Ag_length(keel_slice_keel_routine_slot_ag2_Ag s) { return s.len; }
static inline keel_routine_slot_ag2_Ag *
keel_slice_keel_routine_slot_ag2_Ag_ptr1(keel_slice_keel_routine_slot_ag2_Ag s, size_t i) { return &s.ptr[i]; }
#endif /* KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H */
