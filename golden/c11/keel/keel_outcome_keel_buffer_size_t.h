/* keel/keel_outcome_keel_buffer_size_t.h — generated from keel/outcome.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_OUTCOME_KEEL_BUFFER_SIZE_T_H
#define KEEL_KEEL_OUTCOME_KEEL_BUFFER_SIZE_T_H
#include "keel/keel_outcome_keel_buffer_size_t.type.h"
#include "keel/keel_outcome.type.h"

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_buffer_size_t_failed(keel_outcome_keel_buffer_size_t e);
#line 22 "keel/outcome.k"
static inline bool keel_outcome_keel_buffer_size_t_ok    (keel_outcome_keel_buffer_size_t e);
#line 23 "keel/outcome.k"
static inline i32  keel_outcome_keel_buffer_size_t_code  (keel_outcome_keel_buffer_size_t e);
#line 24 "keel/outcome.k"
static inline keel_buffer_size_t keel_outcome_keel_buffer_size_t_value (keel_outcome_keel_buffer_size_t e);
#line 25 "keel/outcome.k"
static inline void keel_outcome_keel_buffer_size_t_value1(keel_outcome_keel_buffer_size_t *r, keel_buffer_size_t v);
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_buffer_size_t keel_outcome_keel_buffer_size_t_win (keel_outcome_keel_buffer_size_t *r);
#line 30 "keel/outcome.k"
static inline keel_outcome_keel_buffer_size_t keel_outcome_keel_buffer_size_t_win1(keel_outcome_keel_buffer_size_t *r, keel_buffer_size_t v);
#line 31 "keel/outcome.k"
static inline keel_outcome_keel_buffer_size_t keel_outcome_keel_buffer_size_t_fail(keel_outcome_keel_buffer_size_t *r, i32 c);
#line 32 "keel/outcome.k"
static inline keel_outcome_keel_buffer_size_t keel_outcome_keel_buffer_size_t_none(keel_outcome_keel_buffer_size_t *r);

#line 21 "keel/outcome.k"
static inline bool keel_outcome_keel_buffer_size_t_failed(keel_outcome_keel_buffer_size_t e) { return e.code != keel_outcome_OK; }
#line 22 "keel/outcome.k"
static inline bool keel_outcome_keel_buffer_size_t_ok    (keel_outcome_keel_buffer_size_t e) { return e.code == keel_outcome_OK; }
#line 23 "keel/outcome.k"
static inline i32  keel_outcome_keel_buffer_size_t_code  (keel_outcome_keel_buffer_size_t e) { return e.code; }
#line 24 "keel/outcome.k"
static inline keel_buffer_size_t keel_outcome_keel_buffer_size_t_value (keel_outcome_keel_buffer_size_t e) { return e.value; }
#line 25 "keel/outcome.k"
static inline void keel_outcome_keel_buffer_size_t_value1(keel_outcome_keel_buffer_size_t *r, keel_buffer_size_t v) { r->value = v; }
#line 29 "keel/outcome.k"
static inline keel_outcome_keel_buffer_size_t keel_outcome_keel_buffer_size_t_win (keel_outcome_keel_buffer_size_t *r)        { r->code = keel_outcome_OK; return *r; }
#line 30 "keel/outcome.k"
static inline keel_outcome_keel_buffer_size_t keel_outcome_keel_buffer_size_t_win1(keel_outcome_keel_buffer_size_t *r, keel_buffer_size_t v) { r->code = keel_outcome_OK; r->value = v; return *r; }
#line 31 "keel/outcome.k"
static inline keel_outcome_keel_buffer_size_t keel_outcome_keel_buffer_size_t_fail(keel_outcome_keel_buffer_size_t *r, i32 c) { r->code = c; return *r; }
#line 32 "keel/outcome.k"
static inline keel_outcome_keel_buffer_size_t keel_outcome_keel_buffer_size_t_none(keel_outcome_keel_buffer_size_t *r)        { r->code = keel_outcome_NONE; return *r; }
#endif /* KEEL_KEEL_OUTCOME_KEEL_BUFFER_SIZE_T_H */
