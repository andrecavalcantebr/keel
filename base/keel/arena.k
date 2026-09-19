/*
 * DESC: keel arena — region-based storage (spec §5.2, backend §5.4)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.arena;

// `arena` is an ordinary type, not a modifier — it is passed by reference
// by the same rule as the verbs below, which does not make it `byref` (spec
// §4.3).
pub typedef struct { size_t top, cap; u8 *ptr; } arena;

// `from_stack(a, N)` creates automatic storage in the caller's scope and
// cannot be reduced to a function: `N` becomes the size of a local array.
// It is plain C passing opaquely through this file, like any user macro.
#define from_stack(a, N) u8 keel__arr[N]; from_array((a), keel__arr, (N))

// `alloc(a, T, n)` reads `T` at the call site to get `sizeof`/`_Alignof`;
// no ordinary function can do that for a `T` chosen at each use.
#define alloc(a, T, n) ((T *)alloc_((a), sizeof(T), _Alignof(T), (n)))

pub inline bool from_array(arena *a, u8 arr[], size_t len) {
    *a = (arena){0};
    if (arr && len > 0) {
        a->cap = len;
        a->ptr = arr;
    }
    return a->cap > 0;
}

pub inline bool from_memory(arena *a, u8 *ptr, size_t len) {
    *a = (arena){0};
    if (ptr && len > 0) {
        a->cap = len;
        a->ptr = ptr;
    }
    return a->cap > 0;
}

pub inline bool from_parent(arena *a, arena *parent, size_t n) {
    *a = (arena){0};
    u8 *base = alloc(parent, u8, n);
    if (base) {
        a->cap = n;
        a->ptr = base;
    }
    return a->cap > 0;
}

// aligns the ADDRESS, not the offset — the base may be anywhere (backend
// §5.4). An overflowing `n * sz` returns NULL (§5.2 §5, backend diagnostic
// `alloc-overflow`).
priv inline void *alloc_(arena *a, size_t sz, size_t align, size_t n) {
    if (sz == 0 || n > SIZE_MAX / sz) return NULL;
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->ptr + a->top);
    size_t pad = (size_t)(-(uintptr_t)base & (align - 1));
    size_t avail = a->cap - a->top;
    if (pad > avail || need > avail - pad) return NULL;
    a->top += pad + need;
    return a->ptr + a->top - need;
}

pub inline size_t mark(const arena *a) { return a->top; }
pub inline void   restore(arena *a, size_t m) { if (m <= a->top) a->top = m; }
pub inline void   reset(arena *a) { a->top = 0; }
pub inline size_t length(const arena *a) { return a->top; }
pub inline size_t capacity(const arena *a) { return a->cap; }
