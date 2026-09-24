/* keel/keel_slice_keel_routine_slot_ag2_Ag.h — generated from keel/slice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#define KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_slice.type.h"
#include "keel/keel_outcome_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_outcome_keel_slice_keel_routine_slot_ag2_Ag.type.h"

typedef struct keel_arena keel_arena;

#line 25 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_from(keel_routine_slot_ag2_Ag *p, size_t n);
#line 29 "keel/slice.k"
static inline size_t keel_slice_keel_routine_slot_ag2_Ag_length(keel_slice_keel_routine_slot_ag2_Ag s);
#line 30 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag    keel_slice_keel_routine_slot_ag2_Ag_get (keel_slice_keel_routine_slot_ag2_Ag s, size_t i);
#line 31 "keel/slice.k"
static inline void   keel_slice_keel_routine_slot_ag2_Ag_set (keel_slice_keel_routine_slot_ag2_Ag s, size_t i, keel_routine_slot_ag2_Ag v);
#line 32 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_ptr (keel_slice_keel_routine_slot_ag2_Ag s);
#line 33 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_ptr1(keel_slice_keel_routine_slot_ag2_Ag s, size_t i);
#line 36 "keel/slice.k"
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_at(keel_slice_keel_routine_slot_ag2_Ag s, size_t i);
#line 43 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_of(keel_slice_keel_routine_slot_ag2_Ag s, size_t a, size_t b);
#line 49 "keel/slice.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_clone(keel_arena *a, keel_slice_keel_routine_slot_ag2_Ag s);
#line 58 "keel/slice.k"
static inline keel_slice_cursor keel_slice_keel_routine_slot_ag2_Ag_begin(keel_slice_keel_routine_slot_ag2_Ag s);
#line 59 "keel/slice.k"
static inline bool keel_slice_keel_routine_slot_ag2_Ag_has_next(keel_slice_keel_routine_slot_ag2_Ag s, keel_slice_cursor *c);
#line 60 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag *keel_slice_keel_routine_slot_ag2_Ag_next(keel_slice_keel_routine_slot_ag2_Ag s, keel_slice_cursor *c);
#line 64 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_partition(keel_slice_keel_routine_slot_ag2_Ag s, size_t k, size_t w);
#include "keel/keel_arena.h"
#include "keel/keel_outcome_keel_routine_slot_ag2_Ag.h"
#include "keel/keel_outcome_keel_slice_keel_routine_slot_ag2_Ag.h"

#line 25 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_from(keel_routine_slot_ag2_Ag *p, size_t n) {
    return (keel_slice_keel_routine_slot_ag2_Ag){ p ? n : 0, p };
}
#line 29 "keel/slice.k"
static inline size_t keel_slice_keel_routine_slot_ag2_Ag_length(keel_slice_keel_routine_slot_ag2_Ag s) { return s.len; }
#line 30 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag    keel_slice_keel_routine_slot_ag2_Ag_get (keel_slice_keel_routine_slot_ag2_Ag s, size_t i) { return s.ptr[i]; }
#line 31 "keel/slice.k"
static inline void   keel_slice_keel_routine_slot_ag2_Ag_set (keel_slice_keel_routine_slot_ag2_Ag s, size_t i, keel_routine_slot_ag2_Ag v) { s.ptr[i] = v; }
#line 32 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_ptr (keel_slice_keel_routine_slot_ag2_Ag s) { return s.ptr; }
#line 33 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_ptr1(keel_slice_keel_routine_slot_ag2_Ag s, size_t i) { return &s.ptr[i]; }
#line 36 "keel/slice.k"
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_at(keel_slice_keel_routine_slot_ag2_Ag s, size_t i) {
    keel_outcome_keel_routine_slot_ag2_Ag r = {0};
    return i < s.len ? keel_outcome_keel_routine_slot_ag2_Ag_win1(&r, s.ptr[i]) : keel_outcome_keel_routine_slot_ag2_Ag_none(&r);
}
#line 43 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_of(keel_slice_keel_routine_slot_ag2_Ag s, size_t a, size_t b) {
    if (b > s.len) b = s.len; if (a > b) a = b;
    return (keel_slice_keel_routine_slot_ag2_Ag){ b - a, s.ptr + a };
}
#line 49 "keel/slice.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_clone(keel_arena *a, keel_slice_keel_routine_slot_ag2_Ag s) {
    keel_outcome_keel_slice_keel_routine_slot_ag2_Ag r = {0};
    keel_routine_slot_ag2_Ag *data = (keel_routine_slot_ag2_Ag *)keel_arena_alloc2(a, sizeof(keel_routine_slot_ag2_Ag), alignof(keel_routine_slot_ag2_Ag), s.len);
    if (!data) return keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_none(&r);
    if (s.len > 0) memcpy(data, s.ptr, s.len * sizeof(keel_routine_slot_ag2_Ag));
    return keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_win1(&r, (keel_slice_keel_routine_slot_ag2_Ag){ s.len, data });
}
#line 58 "keel/slice.k"
static inline keel_slice_cursor keel_slice_keel_routine_slot_ag2_Ag_begin(keel_slice_keel_routine_slot_ag2_Ag s) { (void)s; return (keel_slice_cursor){0}; }
#line 59 "keel/slice.k"
static inline bool   keel_slice_keel_routine_slot_ag2_Ag_has_next(keel_slice_keel_routine_slot_ag2_Ag s, keel_slice_cursor *c) { return c->i < s.len; }
#line 60 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_next(keel_slice_keel_routine_slot_ag2_Ag s, keel_slice_cursor *c) { return &s.ptr[c->i++]; }
#line 64 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_partition(keel_slice_keel_routine_slot_ag2_Ag s, size_t k, size_t w) {
    if (k == 0) return (keel_slice_keel_routine_slot_ag2_Ag){0, s.ptr};
    size_t step = s.len / k + (s.len % k ? 1 : 0);
    size_t lo = w * step;
    if (lo >= s.len) return (keel_slice_keel_routine_slot_ag2_Ag){0, s.ptr};
    size_t hi = lo + step;
    if (hi > s.len) hi = s.len;
    return (keel_slice_keel_routine_slot_ag2_Ag){ hi - lo, s.ptr + lo };
}
#endif /* KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H */
