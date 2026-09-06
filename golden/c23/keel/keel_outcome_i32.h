/* keel/keel_outcome_i32.h — instância de `outcome i32` (backend §5.12). */
#ifndef KEEL_OUTCOME_I32_H
#define KEEL_OUTCOME_I32_H
#include "keel/prelude.h"
#include "keel/outcome.h"
typedef struct keel_outcome_i32 { i32 code; i32 v; } keel_outcome_i32;
static inline bool keel_outcome_i32_failed(keel_outcome_i32 e) { return e.code != keel_outcome_OK; }
static inline bool keel_outcome_i32_ok    (keel_outcome_i32 e) { return e.code == keel_outcome_OK; }
static inline i32  keel_outcome_i32_value (keel_outcome_i32 e) { return e.v; }
static inline i32  keel_outcome_i32_code  (keel_outcome_i32 e) { return e.code; }
static inline keel_outcome_i32 keel_outcome_i32_win (i32 v) { return (keel_outcome_i32){ keel_outcome_OK, v }; }
static inline keel_outcome_i32 keel_outcome_i32_fail(i32 c) { return (keel_outcome_i32){ c, (i32){0} }; }
static inline keel_outcome_i32 keel_outcome_i32_none(void)  { return (keel_outcome_i32){ keel_outcome_NONE, (i32){0} }; }
#endif
