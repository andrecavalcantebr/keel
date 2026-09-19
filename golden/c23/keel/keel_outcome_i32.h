/* keel/keel_outcome_i32.impl.h — instância de `outcome i32` (backend §5.12). */
#ifndef KEEL_KEEL_OUTCOME_I32_IMPL_H
#define KEEL_KEEL_OUTCOME_I32_IMPL_H
#include "keel/keel_outcome_i32.h"
#include "keel/keel_outcome.h"

static inline bool keel_outcome_i32_failed(keel_outcome_i32 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_i32_ok    (keel_outcome_i32 e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_i32_value (keel_outcome_i32 e) { return e.v; }
static inline void keel_outcome_i32_value1(keel_outcome_i32 *e, i32 v) { e->v = v; }
static inline i32  keel_outcome_i32_code  (keel_outcome_i32 e) { return e.code; }
/* Escritas recebem o objeto, ajustam-no e devolvem seu conteúdo. */
static inline keel_outcome_i32 keel_outcome_i32_win(keel_outcome_i32 *r) { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_i32 keel_outcome_i32_win1(keel_outcome_i32 *r, i32 v) { r->code = keel_outcome_OK; r->v = v; return *r; }
static inline keel_outcome_i32 keel_outcome_i32_fail(keel_outcome_i32 *r, i32 c) { r->code = c; return *r; }
static inline keel_outcome_i32 keel_outcome_i32_none(keel_outcome_i32 *r) { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_I32_IMPL_H */
