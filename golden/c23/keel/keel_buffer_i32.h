/* keel/keel_buffer_i32.h — instância de `buffer i32` (backend §5.2, §5.13). */
#ifndef KEEL_BUFFER_I32_H
#define KEEL_BUFFER_I32_H
#include "keel/prelude.h"
#include "keel/keel_slice_i32.h"
#include "keel/keel_outcome_i32.h"

typedef struct keel_buffer_i32 { size_t cap; size_t len; i32 *ptr; } keel_buffer_i32;

/* `buffer.from(p,cap)`: ponteiro cru — nasce vazio  (linguagem §4.5) */
static inline keel_buffer_i32 keel_buffer_i32_as(i32 *p, size_t n) {
    return (keel_buffer_i32){ .cap = p ? n : 0, .len = 0, .ptr = p };
}
/* `buffer.of(v)`: sobre `array` — nasce cheio, `len == cap` */
static inline keel_buffer_i32 keel_buffer_i32_of(i32 *p, size_t n) {
    return (keel_buffer_i32){ .cap = p ? n : 0, .len = p ? n : 0, .ptr = p };
}
static inline size_t keel_buffer_i32_length  (const keel_buffer_i32 *b) { return b->len; }
static inline size_t keel_buffer_i32_capacity(const keel_buffer_i32 *b) { return b->cap; }
static inline i32    keel_buffer_i32_get(const keel_buffer_i32 *b, size_t i) { return b->ptr[i]; }
static inline void   keel_buffer_i32_set(keel_buffer_i32 *b, size_t i, i32 v) { b->ptr[i] = v; }
static inline i32   *keel_buffer_i32_ptr (const keel_buffer_i32 *b) { return b->ptr; }
static inline i32   *keel_buffer_i32_ptr1(const keel_buffer_i32 *b, size_t i) { return &b->ptr[i]; }
static inline i32   *keel_buffer_i32_push(keel_buffer_i32 *b) {
    if (b->len == b->cap) return NULL;
    return &b->ptr[b->len++];
}
static inline i32   *keel_buffer_i32_push1(keel_buffer_i32 *b, i32 v) {
    if (b->len == b->cap) return NULL;
    b->ptr[b->len] = v;
    return &b->ptr[b->len++];
}
static inline i32   *keel_buffer_i32_pop(keel_buffer_i32 *b) {
    if (b->len == 0) return NULL;
    return &b->ptr[--b->len];
}
static inline void   keel_buffer_i32_clear(keel_buffer_i32 *b) { b->len = 0; }
/* único acessor com teste em release; devolve `outcome i32` (backend §5.13) */
static inline keel_outcome_i32 keel_buffer_i32_at(const keel_buffer_i32 *b, size_t i) {
    return i < b->len ? keel_outcome_i32_win(b->ptr[i]) : keel_outcome_i32_none();
}
static inline keel_slice_i32 keel_buffer_i32_as_slice(const keel_buffer_i32 *b) {
    return (keel_slice_i32){ b->len, b->ptr };
}
#endif
