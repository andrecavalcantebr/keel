/* keel/keel_buffer_i32.h — generated from keel/buffer.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BUFFER_I32_H
#define KEEL_KEEL_BUFFER_I32_H
#include "keel/keel_buffer_i32.type.h"
#include "keel/keel_buffer.type.h"
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_outcome_i32.type.h"

/* `buffer.from(p,cap)`: ponteiro cru — nasce vazio  (linguagem §4.5) */
static inline keel_buffer_i32 keel_buffer_i32_from(i32 *p, size_t n);
/* `buffer.of(v)`: sobre `array` — nasce cheio, `len == cap` */
static inline keel_buffer_i32 keel_buffer_i32_of(i32 *p, size_t n);
static inline size_t keel_buffer_i32_length  (const keel_buffer_i32 *b);
static inline size_t keel_buffer_i32_capacity(const keel_buffer_i32 *b);
static inline i32    keel_buffer_i32_get(const keel_buffer_i32 *b, size_t i);
static inline void   keel_buffer_i32_set(keel_buffer_i32 *b, size_t i, i32 v);
static inline i32   *keel_buffer_i32_ptr (const keel_buffer_i32 *b);
static inline i32   *keel_buffer_i32_ptr1(const keel_buffer_i32 *b, size_t i);
static inline i32   *keel_buffer_i32_push(keel_buffer_i32 *b);
static inline i32   *keel_buffer_i32_push1(keel_buffer_i32 *b, i32 v);
static inline i32   *keel_buffer_i32_pop(keel_buffer_i32 *b);
static inline void   keel_buffer_i32_clear(keel_buffer_i32 *b);
/* único acessor com teste em release; devolve `outcome i32` (backend §5.13) */
static inline keel_outcome_i32 keel_buffer_i32_at(const keel_buffer_i32 *b, size_t i);
static inline keel_slice_i32 keel_buffer_i32_as_slice(const keel_buffer_i32 *b);
/* cursor: `walk` pede `begin`, `has_next` e `next`; `next` devolve o endereço
   do elemento, então o binder é por ponteiro (linguagem §5.3) */
static inline keel_buffer_cursor keel_buffer_i32_begin(const keel_buffer_i32 *b);
static inline bool keel_buffer_i32_has_next(const keel_buffer_i32 *b, keel_buffer_cursor *c);
static inline i32 *keel_buffer_i32_next(const keel_buffer_i32 *b, keel_buffer_cursor *c);
/* partição: as `k` partes são disjuntas e cobrem o contêiner; a de índice alto
   pode sair vazia. O produto é `slice i32` (linguagem §5.3) */
static inline keel_slice_i32 keel_buffer_i32_partition(const keel_buffer_i32 *b, size_t k, size_t w);
/* `slice.of(x,a,b)` — sufixo 2: dois argumentos além do contêiner (§2.1).
   Sem ele colidiria com o de cima; C não tem sobrecarga. */
static inline keel_slice_i32 keel_buffer_i32_as_slice2(const keel_buffer_i32 *b, size_t a, size_t c);
#include "keel/keel_outcome_i32.h"

/* `buffer.from(p,cap)`: ponteiro cru — nasce vazio  (linguagem §4.5) */
static inline keel_buffer_i32 keel_buffer_i32_from(i32 *p, size_t n) {
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
    keel_outcome_i32 r = {0};
    return i < b->len ? keel_outcome_i32_win1(&r, b->ptr[i]) : keel_outcome_i32_none(&r);
}
static inline keel_slice_i32 keel_buffer_i32_as_slice(const keel_buffer_i32 *b) {
    return (keel_slice_i32){ b->len, b->ptr };
}
/* cursor: `walk` pede `begin`, `has_next` e `next`; `next` devolve o endereço
   do elemento, então o binder é por ponteiro (linguagem §5.3) */
static inline keel_buffer_cursor keel_buffer_i32_begin(const keel_buffer_i32 *b) {
    (void)b; return (keel_buffer_cursor){ 0 };
}
static inline bool keel_buffer_i32_has_next(const keel_buffer_i32 *b, keel_buffer_cursor *c) {
    return c->i < b->len;
}
static inline i32 *keel_buffer_i32_next(const keel_buffer_i32 *b, keel_buffer_cursor *c) {
    return &b->ptr[c->i++];
}
/* partição: as `k` partes são disjuntas e cobrem o contêiner; a de índice alto
   pode sair vazia. O produto é `slice i32` (linguagem §5.3) */
static inline keel_slice_i32 keel_buffer_i32_partition(const keel_buffer_i32 *b, size_t k, size_t w) {
    size_t n = b->len, step = k ? (n + k - 1) / k : n;
    size_t lo = w * step, hi;
    if (lo > n) lo = n;
    hi = lo + step; if (hi > n) hi = n;
    return (keel_slice_i32){ hi - lo, b->ptr + lo };
}
/* `slice.of(x,a,b)` — sufixo 2: dois argumentos além do contêiner (§2.1).
   Sem ele colidiria com o de cima; C não tem sobrecarga. */
static inline keel_slice_i32 keel_buffer_i32_as_slice2(const keel_buffer_i32 *b, size_t a, size_t c) {
    return (keel_slice_i32){ c - a, b->ptr + a };
}
#endif /* KEEL_KEEL_BUFFER_I32_H */
