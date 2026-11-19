/* keel/keel_outcome_keel_bitslice_1_bool.h — generated from keel/outcome.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_BITSLICE_1_BOOL_H
#define KEEL_KEEL_OUTCOME_KEEL_BITSLICE_1_BOOL_H

#include "keel/keel_outcome_keel_bitslice_1_bool.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_bitslice_1_bool_failed(keel_outcome_keel_bitslice_1_bool e);
static inline bool keel_outcome_keel_bitslice_1_bool_ok    (keel_outcome_keel_bitslice_1_bool e);
static inline i32  keel_outcome_keel_bitslice_1_bool_code  (keel_outcome_keel_bitslice_1_bool e);
static inline keel_bitslice_1_bool    keel_outcome_keel_bitslice_1_bool_value (keel_outcome_keel_bitslice_1_bool e);
static inline void keel_outcome_keel_bitslice_1_bool_value1 (keel_outcome_keel_bitslice_1_bool *r, keel_bitslice_1_bool v);
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_bitslice_1_bool keel_outcome_keel_bitslice_1_bool_win (keel_outcome_keel_bitslice_1_bool *r);
static inline keel_outcome_keel_bitslice_1_bool keel_outcome_keel_bitslice_1_bool_win1 (keel_outcome_keel_bitslice_1_bool *r, keel_bitslice_1_bool v);
static inline keel_outcome_keel_bitslice_1_bool keel_outcome_keel_bitslice_1_bool_fail(keel_outcome_keel_bitslice_1_bool *r, i32 c);
static inline keel_outcome_keel_bitslice_1_bool keel_outcome_keel_bitslice_1_bool_none(keel_outcome_keel_bitslice_1_bool *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_bitslice_1_bool_failed(keel_outcome_keel_bitslice_1_bool e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_keel_bitslice_1_bool_ok    (keel_outcome_keel_bitslice_1_bool e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_keel_bitslice_1_bool_code  (keel_outcome_keel_bitslice_1_bool e) { return e.code; }
static inline keel_bitslice_1_bool    keel_outcome_keel_bitslice_1_bool_value (keel_outcome_keel_bitslice_1_bool e) { return e.value; }
static inline void keel_outcome_keel_bitslice_1_bool_value1 (keel_outcome_keel_bitslice_1_bool *r, keel_bitslice_1_bool v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_bitslice_1_bool keel_outcome_keel_bitslice_1_bool_win (keel_outcome_keel_bitslice_1_bool *r)         { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_keel_bitslice_1_bool keel_outcome_keel_bitslice_1_bool_win1 (keel_outcome_keel_bitslice_1_bool *r, keel_bitslice_1_bool v)    { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_keel_bitslice_1_bool keel_outcome_keel_bitslice_1_bool_fail(keel_outcome_keel_bitslice_1_bool *r, i32 c)  { r->code = c; return *r; }
static inline keel_outcome_keel_bitslice_1_bool keel_outcome_keel_bitslice_1_bool_none(keel_outcome_keel_bitslice_1_bool *r)         { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_KEEL_BITSLICE_1_BOOL_H */
