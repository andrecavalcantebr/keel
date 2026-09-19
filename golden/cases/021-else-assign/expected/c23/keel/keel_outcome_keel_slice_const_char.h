/* keel/keel_outcome_keel_slice_const_char.h — gerado de keel/slice.k pelo cgen, perfil C23. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_SLICE_CONST_CHAR_H
#define KEEL_KEEL_OUTCOME_KEEL_SLICE_CONST_CHAR_H
#include "keel/keel_outcome_keel_slice_const_char.type.h"
#include "keel/keel_outcome.type.h"

static inline bool keel_outcome_keel_slice_const_char_failed(keel_outcome_keel_slice_const_char e);
static inline keel_slice_const_char keel_outcome_keel_slice_const_char_value(keel_outcome_keel_slice_const_char e);
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_win1(keel_outcome_keel_slice_const_char *r, keel_slice_const_char v);
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_none(keel_outcome_keel_slice_const_char *r);

static inline bool keel_outcome_keel_slice_const_char_failed(keel_outcome_keel_slice_const_char e) { return e.code != keel_outcome_OK; }
static inline keel_slice_const_char keel_outcome_keel_slice_const_char_value(keel_outcome_keel_slice_const_char e) { return e.value; }
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_win1(keel_outcome_keel_slice_const_char *r, keel_slice_const_char v) { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_none(keel_outcome_keel_slice_const_char *r) { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_KEEL_SLICE_CONST_CHAR_H */
