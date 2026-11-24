/* keel/keel_outcome_app_pl_Particle.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_APP_PL_PARTICLE_H
#define KEEL_KEEL_OUTCOME_APP_PL_PARTICLE_H

#include "keel/keel_outcome_app_pl_Particle.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_app_pl_Particle_failed(keel_outcome_app_pl_Particle e);
static inline bool keel_outcome_app_pl_Particle_ok    (keel_outcome_app_pl_Particle e);
static inline i32  keel_outcome_app_pl_Particle_code  (keel_outcome_app_pl_Particle e);
static inline struct app_pl_Particle    keel_outcome_app_pl_Particle_value (keel_outcome_app_pl_Particle e);
static inline void keel_outcome_app_pl_Particle_value1 (keel_outcome_app_pl_Particle *r, struct app_pl_Particle v);
#line 29 "keel/outcome.k"
static inline keel_outcome_app_pl_Particle keel_outcome_app_pl_Particle_win (keel_outcome_app_pl_Particle *r);
static inline keel_outcome_app_pl_Particle keel_outcome_app_pl_Particle_win1 (keel_outcome_app_pl_Particle *r, struct app_pl_Particle v);
static inline keel_outcome_app_pl_Particle keel_outcome_app_pl_Particle_fail(keel_outcome_app_pl_Particle *r, i32 c);
static inline keel_outcome_app_pl_Particle keel_outcome_app_pl_Particle_none(keel_outcome_app_pl_Particle *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_app_pl_Particle_failed(keel_outcome_app_pl_Particle e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_app_pl_Particle_ok    (keel_outcome_app_pl_Particle e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_app_pl_Particle_code  (keel_outcome_app_pl_Particle e) { return e.code; }
static inline struct app_pl_Particle    keel_outcome_app_pl_Particle_value (keel_outcome_app_pl_Particle e) { return e.value; }
static inline void keel_outcome_app_pl_Particle_value1 (keel_outcome_app_pl_Particle *r, struct app_pl_Particle v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_app_pl_Particle keel_outcome_app_pl_Particle_win (keel_outcome_app_pl_Particle *r)         { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_app_pl_Particle keel_outcome_app_pl_Particle_win1 (keel_outcome_app_pl_Particle *r, struct app_pl_Particle v)    { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_app_pl_Particle keel_outcome_app_pl_Particle_fail(keel_outcome_app_pl_Particle *r, i32 c)  { r->code = c; return *r; }
static inline keel_outcome_app_pl_Particle keel_outcome_app_pl_Particle_none(keel_outcome_app_pl_Particle *r)         { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_APP_PL_PARTICLE_H */
