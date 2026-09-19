/*
 * module keel.slice type $T
 *
 * DESC: keel generic slice type for parser keel; slice is a view over a region
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3
 */

%type
#include "keel.type.h"
$A

#include <stddef.h>

typedef struct keel_slice_$T {
    size_t len;
    $T *ptr;
} keel_slice_$T;

// o cursor nao menciona o parametro, entao e do modulo e nao da instancia
// (backend 5.11). Esta ferramenta nao separa os dois, entao o guard e manual.
#ifndef KEEL_SLICE_CURSOR
#define KEEL_SLICE_CURSOR
typedef struct keel_slice_cursor { size_t i; } keel_slice_cursor;
#endif

%impl
#include "keel/keel_arena.h"
#include "keel/keel_outcome_$T.h"
#include "keel/keel_outcome_keel_slice_$T.h"

#include <string.h>

static inline keel_slice_$T keel_slice_$T_from($T *ptr, size_t len) {
    keel_slice_$T s;
    s.ptr = ptr;
    s.len = ptr ? len : 0;
    return s;
}

static inline size_t keel_slice_$T_length(const keel_slice_$T *s) {
    return s->len;
}

static inline $T keel_slice_$T_get(const keel_slice_$T *s, size_t index) {
    return s->ptr[index];
}

static inline void keel_slice_$T_set(keel_slice_$T *s, size_t index, $T value) {
    s->ptr[index] = value;
}

static inline $T *keel_slice_$T_ptr(const keel_slice_$T *s) {
    return s->ptr;
}

static inline $T *keel_slice_$T_ptr_1(const keel_slice_$T *s, size_t index) {
    return &s->ptr[index];
}

static inline keel_outcome_$T keel_slice_$T_at(const keel_slice_$T *s, size_t index) {
    keel_outcome_$T r;
    if(index < s->len) {
        keel_outcome_$T_win_1(&r, s->ptr[index]);
    } else {
        keel_outcome_$T_none(&r);
    }
    return r;
}

static inline keel_outcome_keel_slice_$T keel_slice_$T_clone(keel_arena *a, const keel_slice_$T *s) {
    keel_outcome_keel_slice_$T r;
    keel_slice_$T out;
    out.ptr = keel_arena_alloc(a, $T, s->len);
    if(out.ptr == NULL) {
        return keel_outcome_keel_slice_$T_none(&r);
    }
    out.len = s->len;
    if(s->len > 0) {
        memcpy(out.ptr, s->ptr, s->len * sizeof($T));
    }
    return keel_outcome_keel_slice_$T_win_1(&r, out);
}

// travessia por cursor
static inline keel_slice_cursor keel_slice_$T_begin(const keel_slice_$T *s) {
    (void)s;
    return (keel_slice_cursor){ .i = 0 };
}

static inline bool keel_slice_$T_has_next(const keel_slice_$T *s, const keel_slice_cursor *c) {
    return c->i < s->len;
}

static inline $T *keel_slice_$T_next(const keel_slice_$T *s, keel_slice_cursor *c) {
    return &s->ptr[c->i++];
}

// parte w de uma divisao em k partes disjuntas
static inline keel_slice_$T keel_slice_$T_partition(const keel_slice_$T *s, size_t k, size_t w) {
    keel_slice_$T part = { .len = 0, .ptr = s->ptr };
    if(k == 0) return part;

    const size_t step = s->len / k + (s->len % k ? 1 : 0);
    const size_t lo = w * step;
    if(lo >= s->len) return part;

    size_t hi = lo + step;
    if(hi > s->len) hi = s->len;

    part.ptr = s->ptr + lo;
    part.len = hi - lo;
    return part;
}
