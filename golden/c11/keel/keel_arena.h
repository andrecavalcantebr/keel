/* keel/keel_arena.h — gerado de keel/arena.k pelo cgen, perfil C11. */
#ifndef KEEL_KEEL_ARENA_H
#define KEEL_KEEL_ARENA_H
#include "keel/keel_arena.type.h"

/* Alinha o ENDEREÇO, não o deslocamento: a base pode estar em qualquer lugar.
   `uintptr_t` calcula o número de bytes de padding e não fabrica ponteiro —
   o endereço devolvido sai de aritmética de ponteiro dentro do próprio vetor. */
static inline void *keel_arena_alloc_n(keel_arena *a, size_t n,
                                                     size_t sz, size_t align);
static inline bool keel_arena_from_array(keel_arena *a, void *buf, size_t n);
static inline bool keel_arena_from_memory(keel_arena *a, void *p, size_t n);
static inline bool keel_arena_from_parent(keel_arena *s, keel_arena *parent, size_t n);
static inline size_t keel_arena_capacity(const keel_arena *a);
static inline size_t keel_arena_length  (const keel_arena *a);
static inline size_t keel_arena_mark    (const keel_arena *a);
static inline void   keel_arena_reset   (keel_arena *a);
static inline void   keel_arena_restore (keel_arena *a, size_t m);

/* Alinha o ENDEREÇO, não o deslocamento: a base pode estar em qualquer lugar.
   `uintptr_t` calcula o número de bytes de padding e não fabrica ponteiro —
   o endereço devolvido sai de aritmética de ponteiro dentro do próprio vetor. */
static inline void *keel_arena_alloc_n(keel_arena *a, size_t n,
                                                     size_t sz, size_t align) {
    if (n > SIZE_MAX / sz) return NULL;                   /* diagnóstico `alloc-overflow` */
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->buf + a->top);
    size_t pad   = (size_t)(((base + (align - 1)) & ~(uintptr_t)(align - 1)) - base);
    size_t avail = a->cap - a->top;
    if (pad > avail || need > avail - pad) return NULL;
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
static inline bool keel_arena_from_parent(keel_arena *s, keel_arena *parent, size_t n) {
    return keel_arena_from_array(s, keel_arena_alloc_n(parent, n, 1, 1), n);
}
static inline size_t keel_arena_capacity(const keel_arena *a) { return a->cap; }
static inline size_t keel_arena_length  (const keel_arena *a) { return a->top; }
static inline size_t keel_arena_mark    (const keel_arena *a) { return a->top; }
static inline void   keel_arena_reset   (keel_arena *a)           { a->top = 0; }
static inline void   keel_arena_restore (keel_arena *a, size_t m) { a->top = m; }
#endif /* KEEL_KEEL_ARENA_H */
