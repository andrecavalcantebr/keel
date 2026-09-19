/* keel/keel_outcome_u32.h — instância de `outcome u32` (backend §5.12). */
#ifndef KEEL_KEEL_OUTCOME_U32_H
#define KEEL_KEEL_OUTCOME_U32_H
#include "keel/keel_outcome_u32.proto.h"
#include "keel/keel_outcome.proto.h"

static inline bool keel_outcome_u32_failed(keel_outcome_u32 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_u32_ok    (keel_outcome_u32 e) { return e.code == keel_outcome_OK; }
static inline u32  keel_outcome_u32_value (keel_outcome_u32 e) { return e.v; }
static inline void keel_outcome_u32_value1(keel_outcome_u32 *e, u32 v) { e->v = v; }
static inline i32  keel_outcome_u32_code  (keel_outcome_u32 e) { return e.code; }
/* Escritas recebem o objeto, ajustam-no e devolvem seu conteúdo. */
static inline keel_outcome_u32 keel_outcome_u32_win(keel_outcome_u32 *r) { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_u32 keel_outcome_u32_win1(keel_outcome_u32 *r, u32 v) { r->code = keel_outcome_OK; r->v = v; return *r; }
static inline keel_outcome_u32 keel_outcome_u32_fail(keel_outcome_u32 *r, i32 c) { r->code = c; return *r; }
static inline keel_outcome_u32 keel_outcome_u32_none(keel_outcome_u32 *r) { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_U32_H */
