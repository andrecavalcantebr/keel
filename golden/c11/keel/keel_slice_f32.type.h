/* keel/keel_slice_f32.type.h — instância de `slice f32` (backend §5.2).
   `slice` NÃO é `byref`, então os acessores recebem CÓPIA, não ponteiro
   (linguagem §4.11). É o que faz `slice.length(slice.of(b))` aninhar sem
   temporário: `of` devolve valor e `length` consome valor. */
#ifndef KEEL_KEEL_SLICE_F32_TYPE_H
#define KEEL_KEEL_SLICE_F32_TYPE_H
#include "keel.type.h"

typedef struct keel_slice_f32 { size_t len; f32 *ptr; } keel_slice_f32;
#endif /* KEEL_KEEL_SLICE_F32_TYPE_H */
