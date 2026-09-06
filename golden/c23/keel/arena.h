/* keel/arena.h — perfil C23. O `.h` do módulo keel.arena; C comum.
   Normativo: keel-c-backend.md §5.4 e §5.4.1. */
#ifndef KEEL_ARENA_H
#define KEEL_ARENA_H
#include "keel/prelude.h"

typedef struct keel_arena {
    size_t         cap;
    size_t         top;
    unsigned char *buf;
} keel_arena;

/* Alinha o ENDEREÇO, não o deslocamento: a base pode estar em qualquer lugar.
   `uintptr_t` calcula o número de bytes de padding e não fabrica ponteiro —
   o endereço devolvido sai de aritmética de ponteiro dentro do próprio vetor. */
[[nodiscard]] static inline void *keel_arena_alloc_n(keel_arena *a, size_t n,
                                                     size_t sz, size_t align) {
    if (n > SIZE_MAX / sz) return NULL;                   /* diagnóstico 109 */
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->buf + a->top);
    size_t pad   = (size_t)(((base + (align - 1)) & ~(uintptr_t)(align - 1)) - base);
    size_t livre = a->cap - a->top;
    if (pad > livre || need > livre - pad) return NULL;
    a->top += pad + need;
    return a->buf + a->top - need;
}

static inline bool keel_arena_from_array(keel_arena *a, void *buf, size_t n) {
    a->cap = buf ? n : 0; a->top = 0; a->buf = (unsigned char *)buf;
    return a->cap > 0;
}
static inline bool keel_arena_from_memory(keel_arena *a, void *p, size_t n) {
    return keel_arena_from_array(a, p, n);
}
static inline bool keel_arena_from_parent(keel_arena *s, keel_arena *pai, size_t n) {
    return keel_arena_from_array(s, keel_arena_alloc_n(pai, n, 1, 1), n);
}

static inline size_t keel_arena_capacity(const keel_arena *a) { return a->cap; }
static inline size_t keel_arena_length  (const keel_arena *a) { return a->top; }
static inline size_t keel_arena_mark    (const keel_arena *a) { return a->top; }
static inline void   keel_arena_reset   (keel_arena *a)           { a->top = 0; }
static inline void   keel_arena_restore (keel_arena *a, size_t m) { a->top = m; }
#endif
