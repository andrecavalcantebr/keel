/* keel/keel_outcome_keel_slice_const_char.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_SLICE_CONST_CHAR_H
#define KEEL_KEEL_OUTCOME_KEEL_SLICE_CONST_CHAR_H

#include "keel/keel_outcome_keel_slice_const_char.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_slice_const_char_failed(keel_outcome_keel_slice_const_char e);
static inline bool keel_outcome_keel_slice_const_char_ok    (keel_outcome_keel_slice_const_char e);
static inline i32  keel_outcome_keel_slice_const_char_code  (keel_outcome_keel_slice_const_char e);
static inline keel_slice_const_char keel_outcome_keel_slice_const_char_value (keel_outcome_keel_slice_const_char e);
static inline void keel_outcome_keel_slice_const_char_value1(keel_outcome_keel_slice_const_char *r, keel_slice_const_char v);
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_win (keel_outcome_keel_slice_const_char *r);
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_win1(keel_outcome_keel_slice_const_char *r, keel_slice_const_char v);
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_fail(keel_outcome_keel_slice_const_char *r, i32 c);
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_none(keel_outcome_keel_slice_const_char *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_slice_const_char_failed(keel_outcome_keel_slice_const_char e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_keel_slice_const_char_ok    (keel_outcome_keel_slice_const_char e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_keel_slice_const_char_code  (keel_outcome_keel_slice_const_char e) { return e.code; }
static inline keel_slice_const_char keel_outcome_keel_slice_const_char_value (keel_outcome_keel_slice_const_char e) { return e.value; }
static inline void keel_outcome_keel_slice_const_char_value1(keel_outcome_keel_slice_const_char *r, keel_slice_const_char v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_win (keel_outcome_keel_slice_const_char *r)        { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_win1(keel_outcome_keel_slice_const_char *r, keel_slice_const_char v) { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_fail(keel_outcome_keel_slice_const_char *r, i32 c) { r->code = c; return *r; }
static inline keel_outcome_keel_slice_const_char keel_outcome_keel_slice_const_char_none(keel_outcome_keel_slice_const_char *r)        { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_KEEL_SLICE_CONST_CHAR_H */
