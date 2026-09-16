/* keel/keel_arena.h — perfil C23. O `.h` do módulo keel.arena; C comum.
   Normativo: keel-c-backend.md §5.4 e §5.4.1. */
#ifndef KEEL_KEEL_ARENA_H
#define KEEL_KEEL_ARENA_H
#include "keel/keel_arena.type.h"

/* Alinha o ENDEREÇO, não o deslocamento: a base pode estar em qualquer lugar.
   `uintptr_t` calcula o número de bytes de padding e não fabrica ponteiro —
   o endereço devolvido sai de aritmética de ponteiro dentro do próprio vetor. */
[[nodiscard]] static inline void *keel_arena_alloc_n(keel_arena *a, size_t n,
                                                     size_t sz, size_t align);
static inline bool keel_arena_from_array(keel_arena *a, void *buf, size_t n);
static inline bool keel_arena_from_memory(keel_arena *a, void *p, size_t n);
static inline bool keel_arena_from_parent(keel_arena *s, keel_arena *pai, size_t n);
static inline size_t keel_arena_capacity(const keel_arena *a);
static inline size_t keel_arena_length  (const keel_arena *a);
static inline size_t keel_arena_mark    (const keel_arena *a);
static inline void   keel_arena_reset   (keel_arena *a);
static inline void   keel_arena_restore (keel_arena *a, size_t m);
#endif /* KEEL_KEEL_ARENA_H */
