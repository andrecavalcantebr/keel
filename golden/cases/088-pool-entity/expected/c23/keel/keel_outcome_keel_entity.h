/* keel/keel_outcome_keel_entity.h — generated from keel/outcome.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_ENTITY_H
#define KEEL_KEEL_OUTCOME_KEEL_ENTITY_H

#include "keel/keel_outcome_keel_entity.type.h"
#include "keel/keel_outcome.type.h"
#include "keel/keel_entity.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_entity_failed(keel_outcome_keel_entity e);
static inline bool keel_outcome_keel_entity_ok    (keel_outcome_keel_entity e);
static inline i32  keel_outcome_keel_entity_code  (keel_outcome_keel_entity e);
static inline keel_entity    keel_outcome_keel_entity_value (keel_outcome_keel_entity e);
static inline void keel_outcome_keel_entity_value1 (keel_outcome_keel_entity *r, keel_entity v);
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_entity keel_outcome_keel_entity_win (keel_outcome_keel_entity *r);
static inline keel_outcome_keel_entity keel_outcome_keel_entity_win1 (keel_outcome_keel_entity *r, keel_entity v);
static inline keel_outcome_keel_entity keel_outcome_keel_entity_fail(keel_outcome_keel_entity *r, i32 c);
static inline keel_outcome_keel_entity keel_outcome_keel_entity_none(keel_outcome_keel_entity *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_entity_failed(keel_outcome_keel_entity e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_keel_entity_ok    (keel_outcome_keel_entity e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_keel_entity_code  (keel_outcome_keel_entity e) { return e.code; }
static inline keel_entity    keel_outcome_keel_entity_value (keel_outcome_keel_entity e) { return e.value; }
static inline void keel_outcome_keel_entity_value1 (keel_outcome_keel_entity *r, keel_entity v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_entity keel_outcome_keel_entity_win (keel_outcome_keel_entity *r)         { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_keel_entity keel_outcome_keel_entity_win1 (keel_outcome_keel_entity *r, keel_entity v)    { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_keel_entity keel_outcome_keel_entity_fail(keel_outcome_keel_entity *r, i32 c)  { r->code = c; return *r; }
static inline keel_outcome_keel_entity keel_outcome_keel_entity_none(keel_outcome_keel_entity *r)         { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_KEEL_ENTITY_H */
