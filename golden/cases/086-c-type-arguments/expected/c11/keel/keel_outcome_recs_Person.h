/* keel/keel_outcome_recs_Person.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_RECS_PERSON_H
#define KEEL_KEEL_OUTCOME_RECS_PERSON_H

#include "keel/keel_outcome_recs_Person.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_recs_Person_failed(keel_outcome_recs_Person e);
static inline bool keel_outcome_recs_Person_ok    (keel_outcome_recs_Person e);
static inline i32  keel_outcome_recs_Person_code  (keel_outcome_recs_Person e);
static inline struct recs_Person    keel_outcome_recs_Person_value (keel_outcome_recs_Person e);
static inline void keel_outcome_recs_Person_value1 (keel_outcome_recs_Person *r, struct recs_Person v);
#line 29 "keel/outcome.k"
static inline keel_outcome_recs_Person keel_outcome_recs_Person_win (keel_outcome_recs_Person *r);
static inline keel_outcome_recs_Person keel_outcome_recs_Person_win1 (keel_outcome_recs_Person *r, struct recs_Person v);
static inline keel_outcome_recs_Person keel_outcome_recs_Person_fail(keel_outcome_recs_Person *r, i32 c);
static inline keel_outcome_recs_Person keel_outcome_recs_Person_none(keel_outcome_recs_Person *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_recs_Person_failed(keel_outcome_recs_Person e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_recs_Person_ok    (keel_outcome_recs_Person e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_recs_Person_code  (keel_outcome_recs_Person e) { return e.code; }
static inline struct recs_Person    keel_outcome_recs_Person_value (keel_outcome_recs_Person e) { return e.value; }
static inline void keel_outcome_recs_Person_value1 (keel_outcome_recs_Person *r, struct recs_Person v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_recs_Person keel_outcome_recs_Person_win (keel_outcome_recs_Person *r)         { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_recs_Person keel_outcome_recs_Person_win1 (keel_outcome_recs_Person *r, struct recs_Person v)    { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_recs_Person keel_outcome_recs_Person_fail(keel_outcome_recs_Person *r, i32 c)  { r->code = c; return *r; }
static inline keel_outcome_recs_Person keel_outcome_recs_Person_none(keel_outcome_recs_Person *r)         { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_RECS_PERSON_H */
