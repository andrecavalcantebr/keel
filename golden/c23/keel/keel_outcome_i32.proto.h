/* keel/keel_outcome_i32.proto.h — gerado de keel/outcome.k pelo cgen, perfil C23. */
#ifndef KEEL_KEEL_OUTCOME_I32_PROTO_H
#define KEEL_KEEL_OUTCOME_I32_PROTO_H
#include "keel/keel_outcome_i32.type.h"

static inline bool keel_outcome_i32_failed(keel_outcome_i32 e);
static inline bool keel_outcome_i32_ok    (keel_outcome_i32 e);
static inline i32  keel_outcome_i32_value (keel_outcome_i32 e);
static inline void keel_outcome_i32_value1(keel_outcome_i32 *e, i32 v);
static inline i32  keel_outcome_i32_code  (keel_outcome_i32 e);
/* Escritas recebem o objeto, ajustam-no e devolvem seu conteúdo. */
static inline keel_outcome_i32 keel_outcome_i32_win(keel_outcome_i32 *r);
static inline keel_outcome_i32 keel_outcome_i32_win1(keel_outcome_i32 *r, i32 v);
static inline keel_outcome_i32 keel_outcome_i32_fail(keel_outcome_i32 *r, i32 c);
static inline keel_outcome_i32 keel_outcome_i32_none(keel_outcome_i32 *r);
#endif /* KEEL_KEEL_OUTCOME_I32_PROTO_H */
