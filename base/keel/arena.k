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

// There is no `from_stack`: storage in the frame is an `array u8` the program
// declares and hands to `from_array` (spec §5.2, backend §5.4).

pub inline bool from_array(child arena *a, parent array u8 storage[size_t n]) {
    *a = (arena){0};
    if (storage && n > 0) {
        a->cap = n;
        a->ptr = storage;
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

pub inline bool from_parent(child arena *a, parent arena *owner, size_t n) {
    *a = (arena){0};
    u8 *base = alloc(owner, u8, n);
    if (base) {
        a->cap = n;
        a->ptr = base;
    }
    return a->cap > 0;
}

// The raw form: count and element size enter separately, and the product is
// made here — an `n * sz` that overflows returns NULL, and that is the
// `alloc-overflow` diagnostic. Aligns the ADDRESS, not the offset: the region
// may start anywhere.
pub inline child void *alloc(parent arena *a, size_t n, size_t sz, size_t align) {
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

// `type T` is erased (spec §4.4): it becomes size and alignment, and the call
// site writes `sizeof`, `alignof` and the cast (backend §5.16). The arity
// tells the two forms apart.
pub inline child T *alloc(parent arena *a, type T, size_t n) {
    return alloc(a, n, sizeof(T), alignof(T));
}

pub inline size_t mark(const arena *a) { return a->top; }
pub inline void   restore(invalidates arena *a, size_t m) { if (m <= a->top) a->top = m; }
pub inline void   reset(invalidates arena *a) { a->top = 0; }
pub inline size_t length(const arena *a) { return a->top; }
pub inline size_t capacity(const arena *a) { return a->cap; }
