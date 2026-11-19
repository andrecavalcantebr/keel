/* keel/keel_outcome_bool.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_BOOL_H
#define KEEL_KEEL_OUTCOME_BOOL_H

#include "keel/keel_outcome_bool.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_bool_failed(keel_outcome_bool e);
static inline bool keel_outcome_bool_ok    (keel_outcome_bool e);
static inline i32  keel_outcome_bool_code  (keel_outcome_bool e);
static inline bool    keel_outcome_bool_value (keel_outcome_bool e);
static inline void keel_outcome_bool_value1 (keel_outcome_bool *r, bool v);
#line 29 "keel/outcome.k"
static inline keel_outcome_bool keel_outcome_bool_win (keel_outcome_bool *r);
static inline keel_outcome_bool keel_outcome_bool_win1 (keel_outcome_bool *r, bool v);
static inline keel_outcome_bool keel_outcome_bool_fail(keel_outcome_bool *r, i32 c);
static inline keel_outcome_bool keel_outcome_bool_none(keel_outcome_bool *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_bool_failed(keel_outcome_bool e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_bool_ok    (keel_outcome_bool e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_bool_code  (keel_outcome_bool e) { return e.code; }
static inline bool    keel_outcome_bool_value (keel_outcome_bool e) { return e.value; }
static inline void keel_outcome_bool_value1 (keel_outcome_bool *r, bool v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_bool keel_outcome_bool_win (keel_outcome_bool *r)         { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_bool keel_outcome_bool_win1 (keel_outcome_bool *r, bool v)    { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_bool keel_outcome_bool_fail(keel_outcome_bool *r, i32 c)  { r->code = c; return *r; }
static inline keel_outcome_bool keel_outcome_bool_none(keel_outcome_bool *r)         { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_BOOL_H */
