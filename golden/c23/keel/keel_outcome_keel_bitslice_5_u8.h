/* keel/keel_outcome_keel_bitslice_5_u8.h — generated from keel/outcome.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_BITSLICE_5_U8_H
#define KEEL_KEEL_OUTCOME_KEEL_BITSLICE_5_U8_H

#include "keel/keel_outcome_keel_bitslice_5_u8.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_bitslice_5_u8_failed(keel_outcome_keel_bitslice_5_u8 e);
static inline bool keel_outcome_keel_bitslice_5_u8_ok    (keel_outcome_keel_bitslice_5_u8 e);
static inline i32  keel_outcome_keel_bitslice_5_u8_code  (keel_outcome_keel_bitslice_5_u8 e);
static inline keel_bitslice_5_u8    keel_outcome_keel_bitslice_5_u8_value (keel_outcome_keel_bitslice_5_u8 e);
static inline void keel_outcome_keel_bitslice_5_u8_value1 (keel_outcome_keel_bitslice_5_u8 *r, keel_bitslice_5_u8 v);
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_bitslice_5_u8 keel_outcome_keel_bitslice_5_u8_win (keel_outcome_keel_bitslice_5_u8 *r);
static inline keel_outcome_keel_bitslice_5_u8 keel_outcome_keel_bitslice_5_u8_win1 (keel_outcome_keel_bitslice_5_u8 *r, keel_bitslice_5_u8 v);
static inline keel_outcome_keel_bitslice_5_u8 keel_outcome_keel_bitslice_5_u8_fail(keel_outcome_keel_bitslice_5_u8 *r, i32 c);
static inline keel_outcome_keel_bitslice_5_u8 keel_outcome_keel_bitslice_5_u8_none(keel_outcome_keel_bitslice_5_u8 *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_bitslice_5_u8_failed(keel_outcome_keel_bitslice_5_u8 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_keel_bitslice_5_u8_ok    (keel_outcome_keel_bitslice_5_u8 e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_keel_bitslice_5_u8_code  (keel_outcome_keel_bitslice_5_u8 e) { return e.code; }
static inline keel_bitslice_5_u8    keel_outcome_keel_bitslice_5_u8_value (keel_outcome_keel_bitslice_5_u8 e) { return e.value; }
static inline void keel_outcome_keel_bitslice_5_u8_value1 (keel_outcome_keel_bitslice_5_u8 *r, keel_bitslice_5_u8 v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_bitslice_5_u8 keel_outcome_keel_bitslice_5_u8_win (keel_outcome_keel_bitslice_5_u8 *r)         { r->code = keel_outcome_OK; return *r; }
static inline keel_outcome_keel_bitslice_5_u8 keel_outcome_keel_bitslice_5_u8_win1 (keel_outcome_keel_bitslice_5_u8 *r, keel_bitslice_5_u8 v)    { r->code = keel_outcome_OK; r->value = v; return *r; }
static inline keel_outcome_keel_bitslice_5_u8 keel_outcome_keel_bitslice_5_u8_fail(keel_outcome_keel_bitslice_5_u8 *r, i32 c)  { r->code = c; return *r; }
static inline keel_outcome_keel_bitslice_5_u8 keel_outcome_keel_bitslice_5_u8_none(keel_outcome_keel_bitslice_5_u8 *r)         { r->code = keel_outcome_NONE; return *r; }

#endif /* KEEL_KEEL_OUTCOME_KEEL_BITSLICE_5_U8_H */
