/* keel/keel_arena.h — generated from keel/arena.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_ARENA_H
#define KEEL_KEEL_ARENA_H
#include "keel/keel_arena.type.h"

#line 18 "keel/arena.k"
static inline bool keel_arena_from_array(keel_arena *a, u8 storage[], size_t keel__n);
#line 27 "keel/arena.k"
static inline bool keel_arena_from_memory(keel_arena *a, u8 *ptr, size_t len);
#line 36 "keel/arena.k"
static inline bool keel_arena_from_parent(keel_arena *a, keel_arena *owner, size_t n);
#line 50 "keel/arena.k"
static inline void *keel_arena_alloc3(keel_arena *a, size_t n, size_t sz, size_t align);
#line 65 "keel/arena.k"
static inline void *keel_arena_alloc2(keel_arena *a, size_t keel__T_size, size_t keel__T_align, size_t n);
#line 69 "keel/arena.k"
static inline size_t keel_arena_mark(const keel_arena *a);
static inline void   keel_arena_restore(keel_arena *a, size_t m);
static inline void   keel_arena_reset(keel_arena *a);
static inline size_t keel_arena_length(const keel_arena *a);
static inline size_t keel_arena_capacity(const keel_arena *a);

#line 18 "keel/arena.k"
static inline bool keel_arena_from_array(keel_arena *a, u8 storage[], size_t keel__n) {
    *a = (keel_arena){0};
    if (storage && keel__n > 0) {
        a->cap = keel__n;
        a->ptr = storage;
    }
    return a->cap > 0;
}
#line 27 "keel/arena.k"
static inline bool keel_arena_from_memory(keel_arena *a, u8 *ptr, size_t len) {
    *a = (keel_arena){0};
    if (ptr && len > 0) {
        a->cap = len;
        a->ptr = ptr;
    }
    return a->cap > 0;
}
#line 36 "keel/arena.k"
static inline bool keel_arena_from_parent(keel_arena *a, keel_arena *owner, size_t n) {
    *a = (keel_arena){0};
    u8 *base = (u8 *)keel_arena_alloc2(owner, sizeof(u8), _Alignof(u8), n);
    if (base) {
        a->cap = n;
        a->ptr = base;
    }
    return a->cap > 0;
}
#line 50 "keel/arena.k"
static inline void *keel_arena_alloc3(keel_arena *a, size_t n, size_t sz, size_t align) {
    KEEL_CHECK(sz == 0 || n <= SIZE_MAX / sz, "alloc-overflow");
    if (sz == 0 || n > SIZE_MAX / sz) return NULL;
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->ptr + a->top);
    size_t pad = (size_t)(-(uintptr_t)base & (align - 1));
    size_t avail = a->cap - a->top;
    if (pad > avail || need > avail - pad) return NULL;
    a->top += pad + need;
    return a->ptr + a->top - need;
}
#line 65 "keel/arena.k"
static inline void *keel_arena_alloc2(keel_arena *a, size_t keel__T_size, size_t keel__T_align, size_t n) {
    return keel_arena_alloc3(a, n, keel__T_size, keel__T_align);
}
#line 69 "keel/arena.k"
static inline size_t keel_arena_mark(const keel_arena *a) { return a->top; }
static inline void   keel_arena_restore(keel_arena *a, size_t m) { if (m <= a->top) a->top = m; }
static inline void   keel_arena_reset(keel_arena *a) { a->top = 0; }
static inline size_t keel_arena_length(const keel_arena *a) { return a->top; }
static inline size_t keel_arena_capacity(const keel_arena *a) { return a->cap; }
#endif /* KEEL_KEEL_ARENA_H */
