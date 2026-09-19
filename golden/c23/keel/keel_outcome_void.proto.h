/* keel/keel_outcome_void.proto.h — gerado de keel/outcome.k pelo cgen, perfil C23. */
#ifndef KEEL_KEEL_OUTCOME_VOID_PROTO_H
#define KEEL_KEEL_OUTCOME_VOID_PROTO_H
#include "keel/keel_outcome_void.type.h"

static inline bool keel_outcome_void_ok(keel_outcome_void r);
static inline bool keel_outcome_void_failed(keel_outcome_void r);
static inline i32 keel_outcome_void_code(keel_outcome_void r);
/* Escritas recebem o objeto, ajustam-no e devolvem seu conteúdo. */
static inline keel_outcome_void keel_outcome_void_win(keel_outcome_void *r);
static inline keel_outcome_void keel_outcome_void_fail(keel_outcome_void *r, i32 c);
static inline keel_outcome_void keel_outcome_void_none(keel_outcome_void *r);
#endif /* KEEL_KEEL_OUTCOME_VOID_PROTO_H */
