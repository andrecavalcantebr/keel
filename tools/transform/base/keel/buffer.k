/*
 * module keel.buffer type $T
 *
 * DESC: keel generic buffer type for parser keel
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3
 */

%type
#include "keel.type.h"
$A

#include <stddef.h>

typedef struct keel_buffer_$T {
    size_t len, cap;
    $T *ptr;
} keel_buffer_$T;

// o cursor nao menciona o parametro, entao e do modulo e nao da instancia
// (backend 5.11). Esta ferramenta nao separa os dois, entao o guard e manual.
#ifndef KEEL_BUFFER_CURSOR
#define KEEL_BUFFER_CURSOR
typedef struct keel_buffer_cursor { size_t i; } keel_buffer_cursor;
#endif

%impl
#include "keel/keel_arena.h"
#include "keel/keel_slice_$T.type.h"
#include "keel/keel_outcome_$T.h"
#include "keel/keel_outcome_keel_buffer_$T.h"

#include <string.h>

static inline keel_buffer_$T keel_buffer_$T_of($T *ptr, size_t len) {
    keel_buffer_$T b;
    b.ptr = ptr;
    b.len = ptr ? len : 0;
    b.cap = ptr ? len : 0;
    return b;
}

static inline keel_buffer_$T keel_buffer_$T_from($T *ptr, size_t cap) {
    keel_buffer_$T b;
    b.ptr = ptr;
    b.len = 0;
    b.cap = ptr ? cap : 0;
    return b;
}

static inline size_t keel_buffer_$T_length(const keel_buffer_$T *b) {
    return b->len;
}

static inline size_t keel_buffer_$T_capacity(const keel_buffer_$T *b) {
    return b->cap;
}

static inline $T keel_buffer_$T_get(const keel_buffer_$T *b, size_t index) {
    return b->ptr[index];
}

static inline void keel_buffer_$T_set(keel_buffer_$T *b, size_t index, $T value) {
    b->ptr[index] = value;
}

static inline $T *keel_buffer_$T_ptr(const keel_buffer_$T *b) {
    return b->ptr;
}

static inline $T *keel_buffer_$T_ptr_1(const keel_buffer_$T *b, size_t index) {
    return &b->ptr[index];
}

static inline keel_outcome_$T keel_buffer_$T_at(const keel_buffer_$T *b, size_t index) {
    keel_outcome_$T r;
    if(index < b->len) {
        keel_outcome_$T_win_1(&r, b->ptr[index]);
    } else {
        keel_outcome_$T_none(&r);
    }
    return r;
}

static inline $T *keel_buffer_$T_push(keel_buffer_$T *b) {
    if(b->len < b->cap) {
        return &b->ptr[b->len++];
    }
    return NULL;
}

static inline $T *keel_buffer_$T_push_1(keel_buffer_$T *b, $T value) {
    if(b->len < b->cap) {
        b->ptr[b->len] = value;
        return &b->ptr[b->len++];
    }
    return NULL;
}

static inline $T *keel_buffer_$T_pop(keel_buffer_$T *b) {
    if(b->len > 0) {
        return &b->ptr[--b->len];
    }
    return NULL;
}

static inline void keel_buffer_$T_clear(keel_buffer_$T *b) {
    b->len = 0;
}

static inline keel_outcome_keel_buffer_$T keel_buffer_$T_clone(keel_arena *a, const keel_buffer_$T *b) {
    keel_outcome_keel_buffer_$T r;
    keel_buffer_$T out;
    out.ptr = keel_arena_alloc(a, $T, b->cap);
    if(out.ptr == NULL) {
        return keel_outcome_keel_buffer_$T_none(&r);
    }
    out.len = b->len;
    out.cap = b->cap;
    if(b->len > 0) {
        memcpy(out.ptr, b->ptr, b->len * sizeof($T));
    }
    return keel_outcome_keel_buffer_$T_win_1(&r, out);
}

// o recorte e verbo do buffer, nao do slice: e o buffer que conhece os dois
static inline keel_slice_$T keel_buffer_$T_as_slice(const keel_buffer_$T *b) {
    keel_slice_$T s;
    s.ptr = b->ptr;
    s.len = b->len;
    return s;
}

static inline keel_slice_$T keel_buffer_$T_as_slice_2(const keel_buffer_$T *b, size_t first, size_t limit) {
    keel_slice_$T s = { .len = 0, .ptr = b->ptr };
    if(first > limit || limit > b->len) return s;
    s.ptr = b->ptr + first;
    s.len = limit - first;
    return s;
}

// travessia por cursor
static inline keel_buffer_cursor keel_buffer_$T_begin(const keel_buffer_$T *b) {
    (void)b;
    return (keel_buffer_cursor){ .i = 0 };
}

static inline bool keel_buffer_$T_has_next(const keel_buffer_$T *b, const keel_buffer_cursor *c) {
    return c->i < b->len;
}

static inline $T *keel_buffer_$T_next(const keel_buffer_$T *b, keel_buffer_cursor *c) {
    return &b->ptr[c->i++];
}

// parte w de uma divisao em k partes disjuntas; o produto e slice
static inline keel_slice_$T keel_buffer_$T_partition(const keel_buffer_$T *b, size_t k, size_t w) {
    keel_slice_$T part = { .len = 0, .ptr = b->ptr };
    if(k == 0) return part;

    const size_t step = b->len / k + (b->len % k ? 1 : 0);
    const size_t lo = w * step;
    if(lo >= b->len) return part;

    size_t hi = lo + step;
    if(hi > b->len) hi = b->len;

    part.ptr = b->ptr + lo;
    part.len = hi - lo;
    return part;
}
