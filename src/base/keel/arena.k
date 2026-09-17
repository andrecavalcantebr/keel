/*
 * module keel.arena
 *
 * DESC: keel arena for parser keel
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3
 */

%type
#include "keel.type.h"

#include <stddef.h>

typedef struct keel_arena {
    size_t top, cap;
    u8 *ptr;
} keel_arena;

%h
#define keel_arena_from_stack(a, len) u8 arr[len]; keel_arena_from_array((a), arr, (len))

#define keel_arena_alloc(a, T, len) \
    ((T *)keel_arena_alloc_((a), sizeof(T), _Alignof(T), (len)))

%impl
#include <assert.h>
#include <stdint.h>

static inline bool keel_arena_from_array(keel_arena *a, u8 *arr, size_t len) {
    assert(a && "arena must not be null");
    *a = (keel_arena){0};
    if(arr && len>0) {
        a->top = 0;
        a->cap = len;
        a->ptr = arr;
    }
    return a->cap>0;
}

static inline bool keel_arena_from_memory(keel_arena *a, u8 *ptr, size_t len) {
    assert(a && "arena must not be null");
    *a = (keel_arena){0};
    if(ptr && len>0) {
        a->top = 0;
        a->cap = len;
        a->ptr = ptr;
    }
    return a->cap>0;
}

static inline bool keel_arena_from_parent(keel_arena *a, keel_arena *parent) {
    assert(a && "arena must not be null");
    *a = (keel_arena){0};
    if(parent) {
        a->top = 0;
        a->cap = parent->cap;
        a->ptr = parent->ptr;
    }
    return a->cap>0;
}

static inline void *keel_arena_alloc_(keel_arena *a, size_t datasize, size_t alignment, size_t len) {
    assert(a && "arena must not be null");

    if (datasize == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return NULL;
    }
    if (len > SIZE_MAX / datasize || a->top > a->cap || !a->ptr) {
        return NULL;
    }

    const size_t need = len * datasize;
    const uintptr_t address = (uintptr_t)(a->ptr + a->top);
    const size_t padding = (size_t)(-(uintptr_t)address & ((uintptr_t)alignment - 1));
    const size_t available = a->cap - a->top;

    if (padding > available || need > available - padding) {
        return NULL;
    }

    u8 *result = a->ptr + a->top + padding;
    a->top += padding + need;
    return result;
}

static inline size_t keel_arena_mark(const keel_arena *a) {
    assert(a && "arena must not be null");
    return a->top;
}

static inline void keel_arena_restore(keel_arena *a, size_t mark) {
    assert(a && "arena must not be null");
    if(mark <= a->top)
        a->top = mark;
}

static inline void keel_arena_reset(keel_arena *a) {
    assert(a && "arena must not be null");
    a->top = 0;
}

static inline size_t keel_arena_length(const keel_arena *a) {
    assert(a && "arena must not be null");
    return a->top;
}

static inline size_t keel_arena_capacity(const keel_arena *a) {
    assert(a && "arena must not be null");
    return a->cap;
}
