/* keel/keel_buffer_size_t.impl.h — instância de `buffer size_t`.
   Recortada ao que o caso usa: `of` e `set` (backend §5.2). */
#ifndef KEEL_KEEL_BUFFER_SIZE_T_IMPL_H
#define KEEL_KEEL_BUFFER_SIZE_T_IMPL_H
#include "keel/keel_buffer_size_t.h"

static inline keel_buffer_size_t keel_buffer_size_t_of(size_t *p, size_t n) {
    return (keel_buffer_size_t){ .cap = p ? n : 0, .len = p ? n : 0, .ptr = p };
}
static inline void keel_buffer_size_t_set(keel_buffer_size_t *b, size_t i, size_t v) { b->ptr[i] = v; }
#endif /* KEEL_KEEL_BUFFER_SIZE_T_IMPL_H */
