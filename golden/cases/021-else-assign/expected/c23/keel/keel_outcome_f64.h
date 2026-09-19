/* keel/keel_outcome_f64.h — gerado de keel/outcome.k pelo cgen, perfil C23. */
#ifndef KEEL_KEEL_OUTCOME_F64_H
#define KEEL_KEEL_OUTCOME_F64_H
#include "keel/keel_outcome_f64.type.h"
#include "keel/keel_outcome.type.h"

static inline bool keel_outcome_f64_failed(keel_outcome_f64 e);
static inline f64  keel_outcome_f64_value (keel_outcome_f64 e);
static inline keel_outcome_f64 keel_outcome_f64_win1(keel_outcome_f64 *r, f64 v);
static inline keel_outcome_f64 keel_outcome_f64_none(keel_outcome_f64 *r);

static inline bool keel_outcome_f64_failed(keel_outcome_f64 e) { return e.code != keel_outcome_OK; }
static inline f64  keel_outcome_f64_value (keel_outcome_f64 e) { return e.value; }
static inline keel_outcome_f64 keel_outcome_f64_win1(keel_outcome_f64 *r, f64 v) { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_f64 keel_outcome_f64_none(keel_outcome_f64 *r) { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_F64_H */
