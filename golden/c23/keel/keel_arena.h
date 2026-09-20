/* keel/keel_arena.h — generated from keel/arena.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_ARENA_H
#define KEEL_KEEL_ARENA_H
#include "keel/keel_arena.type.h"

#line 19 "keel/arena.k"
static inline bool keel_arena_from_array(keel_arena *a, u8 arr[], size_t len);
#line 28 "keel/arena.k"
static inline bool keel_arena_from_memory(keel_arena *a, u8 *ptr, size_t len);
#line 37 "keel/arena.k"
static inline bool keel_arena_from_parent(keel_arena *a, keel_arena *parent, size_t n);
#line 53 "keel/arena.k"
[[nodiscard]] static inline void *keel_arena_alloc(keel_arena *a, size_t n, size_t sz, size_t align);
#line 64 "keel/arena.k"
static inline size_t keel_arena_mark(const keel_arena *a);
#line 65 "keel/arena.k"
static inline void   keel_arena_restore(keel_arena *a, size_t m);
#line 66 "keel/arena.k"
static inline void   keel_arena_reset(keel_arena *a);
#line 67 "keel/arena.k"
static inline size_t keel_arena_length(const keel_arena *a);
#line 68 "keel/arena.k"
static inline size_t keel_arena_capacity(const keel_arena *a);

#line 19 "keel/arena.k"
static inline bool keel_arena_from_array(keel_arena *a, u8 arr[], size_t len) {
    *a = (keel_arena){0};
    if (arr && len > 0) {
        a->cap = len;
        a->ptr = arr;
    }
    return a->cap > 0;
}
#line 28 "keel/arena.k"
static inline bool keel_arena_from_memory(keel_arena *a, u8 *ptr, size_t len) {
    *a = (keel_arena){0};
    if (ptr && len > 0) {
        a->cap = len;
        a->ptr = ptr;
    }
    return a->cap > 0;
}
#line 37 "keel/arena.k"
static inline bool keel_arena_from_parent(keel_arena *a, keel_arena *parent, size_t n) {
    *a = (keel_arena){0};
    u8 *base = (u8 *)keel_arena_alloc(parent, n, sizeof(u8), alignof(u8));
    if (base) {
        a->cap = n;
        a->ptr = base;
    }
    return a->cap > 0;
}
#line 53 "keel/arena.k"
[[nodiscard]] static inline void *keel_arena_alloc(keel_arena *a, size_t n, size_t sz, size_t align) {
    if (sz == 0 || n > SIZE_MAX / sz) return NULL;
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->ptr + a->top);
    size_t pad = (size_t)(-(uintptr_t)base & (align - 1));
    size_t avail = a->cap - a->top;
    if (pad > avail || need > avail - pad) return NULL;
    a->top += pad + need;
    return a->ptr + a->top - need;
}
#line 64 "keel/arena.k"
static inline size_t keel_arena_mark(const keel_arena *a) { return a->top; }
#line 65 "keel/arena.k"
static inline void   keel_arena_restore(keel_arena *a, size_t m) { if (m <= a->top) a->top = m; }
#line 66 "keel/arena.k"
static inline void   keel_arena_reset(keel_arena *a) { a->top = 0; }
#line 67 "keel/arena.k"
static inline size_t keel_arena_length(const keel_arena *a) { return a->top; }
#line 68 "keel/arena.k"
static inline size_t keel_arena_capacity(const keel_arena *a) { return a->cap; }
#endif /* KEEL_KEEL_ARENA_H */
