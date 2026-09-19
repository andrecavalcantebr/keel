/* keel/keel_buffer_i32.proto.h — instância de `buffer i32` (backend §5.2, §5.13). */
#ifndef KEEL_KEEL_BUFFER_I32_PROTO_H
#define KEEL_KEEL_BUFFER_I32_PROTO_H
#include "keel/keel_buffer_i32.type.h"
#include "keel/keel_buffer.type.h"
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_outcome_i32.type.h"

/* `buffer.from(p,cap)`: ponteiro cru — nasce vazio  (linguagem §4.5) */
static inline keel_buffer_i32 keel_buffer_i32_as(i32 *p, size_t n);
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
#endif /* KEEL_KEEL_BUFFER_I32_PROTO_H */
