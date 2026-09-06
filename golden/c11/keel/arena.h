/* keel/arena.h — perfil C11. O `.h` do módulo keel.arena; C comum.
   Normativo: keel-c-backend.md §5.4 e §5.4.1. */
#ifndef KEEL_ARENA_H
#define KEEL_ARENA_H
#include "keel/prelude.h"

typedef struct keel_arena {
    size_t         cap;
    size_t         top;
    size_t         base_align;      /* alinhamento efetivo de `buf` */
    unsigned char *buf;
} keel_arena;

static inline void *keel_arena_alloc_n(keel_arena *a, size_t n,
                                                     size_t sz, size_t align) {
    if (align > a->base_align)      return NULL;          /* diagnóstico 82  */
    if (n > SIZE_MAX / sz)          return NULL;          /* diagnóstico 109 */
    size_t need = n * sz;
    size_t p = (a->top + (align - 1)) & ~(align - 1);
    if (p > a->cap || need > a->cap - p) return NULL;
    a->top = p + need;
    return a->buf + p;
}

static inline bool keel_arena_from_array(keel_arena *a, void *buf, size_t n, size_t align) {
    a->cap = buf ? n : 0; a->top = 0; a->base_align = align;
    a->buf = (unsigned char *)buf;
    return a->cap > 0;
}
static inline bool keel_arena_from_memory(keel_arena *a, void *p, size_t n) {
    return keel_arena_from_array(a, p, n, _Alignof(max_align_t));
}
static inline bool keel_arena_from_parent(keel_arena *s, keel_arena *pai, size_t n) {
    void *p = keel_arena_alloc_n(pai, n, 1, pai->base_align);
    return keel_arena_from_array(s, p, n, pai->base_align);
}

static inline size_t keel_arena_capacity(const keel_arena *a) { return a->cap; }
static inline size_t keel_arena_length  (const keel_arena *a) { return a->top; }
static inline size_t keel_arena_mark    (const keel_arena *a) { return a->top; }
static inline void   keel_arena_reset   (keel_arena *a)           { a->top = 0; }
static inline void   keel_arena_restore (keel_arena *a, size_t m) { a->top = m; }
#endif
