/* keel/keel_outcome_void.h — gerado de keel/outcome.k pelo cgen, perfil C11. */
#ifndef KEEL_KEEL_OUTCOME_VOID_H
#define KEEL_KEEL_OUTCOME_VOID_H
#include "keel/keel_outcome_void.proto.h"
#include "keel/keel_outcome.proto.h"

static inline bool keel_outcome_void_ok(keel_outcome_void r) { return r.code == 0; }
static inline bool keel_outcome_void_failed(keel_outcome_void r) { return r.code != 0; }
static inline i32 keel_outcome_void_code(keel_outcome_void r) { return r.code; }
/* Escritas recebem o objeto, ajustam-no e devolvem seu conteúdo. */
static inline keel_outcome_void keel_outcome_void_win(keel_outcome_void *r) { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_void keel_outcome_void_fail(keel_outcome_void *r, i32 c) { r->code = c; return *r; }
static inline keel_outcome_void keel_outcome_void_none(keel_outcome_void *r) { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_VOID_H */
