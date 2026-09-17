/* keel/keel_buffer_size_t.type.h — instância de `buffer size_t`.
   Recortada ao que o caso usa: `of` e `set` (backend §5.2). */
#ifndef KEEL_KEEL_BUFFER_SIZE_T_TYPE_H
#define KEEL_KEEL_BUFFER_SIZE_T_TYPE_H
#include "keel/keel.type.h"

typedef struct keel_buffer_size_t { size_t cap; size_t len; size_t *ptr; } keel_buffer_size_t;
#endif /* KEEL_KEEL_BUFFER_SIZE_T_TYPE_H */
