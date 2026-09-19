/* keel/keel_outcome_u32.proto.h — gerado de keel/outcome.k pelo cgen, perfil C11. */
#ifndef KEEL_KEEL_OUTCOME_U32_PROTO_H
#define KEEL_KEEL_OUTCOME_U32_PROTO_H
#include "keel/keel_outcome_u32.type.h"

static inline bool keel_outcome_u32_failed(keel_outcome_u32 e);
static inline bool keel_outcome_u32_ok    (keel_outcome_u32 e);
static inline u32  keel_outcome_u32_value (keel_outcome_u32 e);
static inline void keel_outcome_u32_value1(keel_outcome_u32 *e, u32 v);
static inline i32  keel_outcome_u32_code  (keel_outcome_u32 e);
/* Escritas recebem o objeto, ajustam-no e devolvem seu conteúdo. */
static inline keel_outcome_u32 keel_outcome_u32_win(keel_outcome_u32 *r);
static inline keel_outcome_u32 keel_outcome_u32_win1(keel_outcome_u32 *r, u32 v);
static inline keel_outcome_u32 keel_outcome_u32_fail(keel_outcome_u32 *r, i32 c);
static inline keel_outcome_u32 keel_outcome_u32_none(keel_outcome_u32 *r);
#endif /* KEEL_KEEL_OUTCOME_U32_PROTO_H */
