/* keel/keel_corot_i32.h — instância de `corot i32` (backend §5.14).
   Mesmo layout de `outcome`, outra leitura do zero: <0 SUCCESS, 0 ONGOING,
   >0 FAILURE. Não há enum de status — FAILURE é região, não valor. */
#ifndef KEEL_COROT_I32_H
#define KEEL_COROT_I32_H
#include "keel/prelude.h"
typedef struct keel_corot_i32 { i32 code; i32 v; } keel_corot_i32;
static inline bool keel_corot_i32_ok     (keel_corot_i32 r) { return r.code <  0; }
static inline bool keel_corot_i32_ongoing(keel_corot_i32 r) { return r.code == 0; }
static inline bool keel_corot_i32_failed (keel_corot_i32 r) { return r.code >  0; }
static inline i32  keel_corot_i32_value  (keel_corot_i32 r) { return r.v; }
static inline i32  keel_corot_i32_code   (keel_corot_i32 r) { return r.code; }
#endif
