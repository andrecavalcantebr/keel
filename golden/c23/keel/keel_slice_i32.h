/* keel/keel_slice_i32.h — instância de `slice i32` (backend §5.2).
   `slice` NÃO é `byref`, então os acessores recebem CÓPIA, não ponteiro
   (linguagem §4.11). É o que faz `slice.length(slice.of(b))` aninhar sem
   temporário: `of` devolve valor e `length` consome valor. */
#ifndef KEEL_SLICE_I32_H
#define KEEL_SLICE_I32_H
#include "keel/prelude.h"
typedef struct keel_slice_i32 { size_t len; i32 *ptr; } keel_slice_i32;

static inline keel_slice_i32 keel_slice_i32_from(i32 *p, size_t n) { return (keel_slice_i32){ p ? n : 0, p }; }
static inline size_t keel_slice_i32_length(keel_slice_i32 s) { return s.len; }
static inline i32    keel_slice_i32_get (keel_slice_i32 s, size_t i) { return s.ptr[i]; }
static inline i32   *keel_slice_i32_ptr (keel_slice_i32 s) { return s.ptr; }
static inline i32   *keel_slice_i32_ptr1(keel_slice_i32 s, size_t i) { return &s.ptr[i]; }
#endif
