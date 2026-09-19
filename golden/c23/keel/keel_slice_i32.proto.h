/* keel/keel_slice_i32.h — instância de `slice i32` (backend §5.2).
   `slice` NÃO é `byref`, então os acessores recebem CÓPIA, não ponteiro
   (linguagem §4.11). É o que faz `slice.length(slice.of(b))` aninhar sem
   temporário: `of` devolve valor e `length` consome valor. */
#ifndef KEEL_KEEL_SLICE_I32_H
#define KEEL_KEEL_SLICE_I32_H
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_slice.type.h"

static inline keel_slice_i32 keel_slice_i32_from(i32 *p, size_t n);
static inline size_t keel_slice_i32_length(keel_slice_i32 s);
static inline i32    keel_slice_i32_get (keel_slice_i32 s, size_t i);
static inline i32   *keel_slice_i32_ptr (keel_slice_i32 s);
static inline i32   *keel_slice_i32_ptr1(keel_slice_i32 s, size_t i);
/* cursor e partição recebem a cópia, como todo verbo de `slice` */
static inline keel_slice_cursor keel_slice_i32_begin(keel_slice_i32 s);
static inline bool keel_slice_i32_has_next(keel_slice_i32 s, keel_slice_cursor *c);
static inline i32 *keel_slice_i32_next(keel_slice_i32 s, keel_slice_cursor *c);
static inline keel_slice_i32 keel_slice_i32_partition(keel_slice_i32 s, size_t k, size_t w);
#endif /* KEEL_KEEL_SLICE_I32_H */
