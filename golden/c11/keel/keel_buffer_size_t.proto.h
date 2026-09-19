/* keel/keel_buffer_size_t.proto.h — instância de `buffer size_t`.
   Recortada ao que o caso usa: `of` e `set` (backend §5.2). */
#ifndef KEEL_KEEL_BUFFER_SIZE_T_PROTO_H
#define KEEL_KEEL_BUFFER_SIZE_T_PROTO_H
#include "keel/keel_buffer_size_t.type.h"
#include "keel/keel_buffer.type.h"

static inline keel_buffer_size_t keel_buffer_size_t_of(size_t *p, size_t n);
static inline void keel_buffer_size_t_set(keel_buffer_size_t *b, size_t i, size_t v);
#endif /* KEEL_KEEL_BUFFER_SIZE_T_PROTO_H */
