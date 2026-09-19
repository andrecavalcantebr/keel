/* keel/keel_buffer_size_t.h — generated from keel/buffer.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_BUFFER_SIZE_T_H
#define KEEL_KEEL_BUFFER_SIZE_T_H
#include "keel/keel_buffer_size_t.type.h"
#include "keel/keel_buffer.type.h"

static inline keel_buffer_size_t keel_buffer_size_t_of(size_t *p, size_t n);
static inline void keel_buffer_size_t_set(keel_buffer_size_t *b, size_t i, size_t v);

static inline keel_buffer_size_t keel_buffer_size_t_of(size_t *p, size_t n) {
    return (keel_buffer_size_t){ .cap = p ? n : 0, .len = p ? n : 0, .ptr = p };
}
static inline void keel_buffer_size_t_set(keel_buffer_size_t *b, size_t i, size_t v) { b->ptr[i] = v; }
#endif /* KEEL_KEEL_BUFFER_SIZE_T_H */
