/* keel/keel_slice_keel_routine_slot_ag2_Ag.h — generated from keel/slice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#define KEEL_KEEL_SLICE_KEEL_ROUTINE_SLOT_AG2_AG_H
#include "keel/keel_slice_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_slice.type.h"
#include "keel/keel_outcome_keel_routine_slot_ag2_Ag.type.h"
#include "keel/keel_outcome_keel_slice_keel_routine_slot_ag2_Ag.type.h"

typedef struct keel_arena keel_arena;

#line 26 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_from(keel_routine_slot_ag2_Ag *p, size_t n);
#line 30 "keel/slice.k"
static inline size_t keel_slice_keel_routine_slot_ag2_Ag_length(keel_slice_keel_routine_slot_ag2_Ag s);
#line 31 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag    keel_slice_keel_routine_slot_ag2_Ag_get (keel_slice_keel_routine_slot_ag2_Ag s, size_t i);
#line 32 "keel/slice.k"
static inline void   keel_slice_keel_routine_slot_ag2_Ag_set (keel_slice_keel_routine_slot_ag2_Ag s, size_t i, keel_routine_slot_ag2_Ag v);
#line 33 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_ptr (keel_slice_keel_routine_slot_ag2_Ag s);
#line 34 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_ptr1(keel_slice_keel_routine_slot_ag2_Ag s, size_t i);
#line 37 "keel/slice.k"
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_at(keel_slice_keel_routine_slot_ag2_Ag s, size_t i);
#line 45 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_as_slice(keel_slice_keel_routine_slot_ag2_Ag s, size_t a, size_t b);
#line 59 "keel/slice.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_clone(keel_arena *a, keel_slice_keel_routine_slot_ag2_Ag s);
#line 68 "keel/slice.k"
static inline keel_slice_cursor keel_slice_keel_routine_slot_ag2_Ag_begin(keel_slice_keel_routine_slot_ag2_Ag s);
#line 69 "keel/slice.k"
static inline bool keel_slice_keel_routine_slot_ag2_Ag_has_next(keel_slice_keel_routine_slot_ag2_Ag s, keel_slice_cursor *c);
#line 70 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag *keel_slice_keel_routine_slot_ag2_Ag_next(keel_slice_keel_routine_slot_ag2_Ag s, keel_slice_cursor *c);
#line 74 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_partition(keel_slice_keel_routine_slot_ag2_Ag s, size_t k, size_t w);
#include "keel/keel_arena.h"
#include "keel/keel_outcome_keel_routine_slot_ag2_Ag.h"
#include "keel/keel_outcome_keel_slice_keel_routine_slot_ag2_Ag.h"

#line 26 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_from(keel_routine_slot_ag2_Ag *p, size_t n) {
    return (keel_slice_keel_routine_slot_ag2_Ag){ p ? n : 0, p };
}
#line 30 "keel/slice.k"
static inline size_t keel_slice_keel_routine_slot_ag2_Ag_length(keel_slice_keel_routine_slot_ag2_Ag s) { return s.len; }
#line 31 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag    keel_slice_keel_routine_slot_ag2_Ag_get (keel_slice_keel_routine_slot_ag2_Ag s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return s.ptr[i]; }
#line 32 "keel/slice.k"
static inline void   keel_slice_keel_routine_slot_ag2_Ag_set (keel_slice_keel_routine_slot_ag2_Ag s, size_t i, keel_routine_slot_ag2_Ag v) { KEEL_CHECK(i < s.len, "set-out-of-length"); s.ptr[i] = v; }
#line 33 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_ptr (keel_slice_keel_routine_slot_ag2_Ag s) { return s.ptr; }
#line 34 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_ptr1(keel_slice_keel_routine_slot_ag2_Ag s, size_t i) { KEEL_CHECK(i < s.len, "index-out-of-length"); return &s.ptr[i]; }
#line 37 "keel/slice.k"
static inline keel_outcome_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_at(keel_slice_keel_routine_slot_ag2_Ag s, size_t i) {
    keel_outcome_keel_routine_slot_ag2_Ag r = {0};
    return i < s.len ? keel_outcome_keel_routine_slot_ag2_Ag_win1(&r, s.ptr[i]) : keel_outcome_keel_routine_slot_ag2_Ag_none(&r);
}
#line 45 "keel/slice.k"
static inline keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_as_slice(keel_slice_keel_routine_slot_ag2_Ag s, size_t a, size_t b) {
    KEEL_CHECK(a <= b && b <= keel_slice_keel_routine_slot_ag2_Ag_length(s), "range-index-out-of-bounds");
    if (b > s.len) b = s.len;
    if (a > b) a = b;
    return (keel_slice_keel_routine_slot_ag2_Ag){ b - a, s.ptr + a };
}
#line 59 "keel/slice.k"
static inline keel_outcome_keel_slice_keel_routine_slot_ag2_Ag keel_slice_keel_routine_slot_ag2_Ag_clone(keel_arena *a, keel_slice_keel_routine_slot_ag2_Ag s) {
    keel_outcome_keel_slice_keel_routine_slot_ag2_Ag r = {0};
    keel_routine_slot_ag2_Ag *data = (keel_routine_slot_ag2_Ag *)keel_arena_alloc2(a, sizeof(keel_routine_slot_ag2_Ag), alignof(keel_routine_slot_ag2_Ag), s.len);
    if (!data) return keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_none(&r);
    if (s.len > 0) memcpy(data, s.ptr, s.len * sizeof(keel_routine_slot_ag2_Ag));
    return keel_outcome_keel_slice_keel_routine_slot_ag2_Ag_win1(&r, (keel_slice_keel_routine_slot_ag2_Ag){ s.len, data });
}
#line 68 "keel/slice.k"
static inline keel_slice_cursor keel_slice_keel_routine_slot_ag2_Ag_begin(keel_slice_keel_routine_slot_ag2_Ag s) { (void)s; return (keel_slice_cursor){0}; }
#line 69 "keel/slice.k"
static inline bool   keel_slice_keel_routine_slot_ag2_Ag_has_next(keel_slice_keel_routine_slot_ag2_Ag s, keel_slice_cursor *c) { return c->i < s.len; }
#line 70 "keel/slice.k"
static inline keel_routine_slot_ag2_Ag   *keel_slice_keel_routine_slot_ag2_Ag_next(keel_slice_keel_routine_slot_ag2_Ag s, keel_slice_cursor *c) { return &s.ptr[c->i++]; }
#line 74 "keel/slice.k"
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
