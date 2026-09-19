/* app/app_pos.c — generated from app/pos.k by cgen, C11 profile. */
#include "app/app_pos.h"
#include "keel/keel_arena.h"
#include "keel/keel_slice_f32.h"
#line 1 "app/pos.k"


















void app_pos_reserve(app_pos_position *p, keel_arena *a, size_t n) {
    p->x = (f32 *)keel_arena_alloc_n(a, n, sizeof(f32), _Alignof(f32));
    p->y = (f32 *)keel_arena_alloc_n(a, n, sizeof(f32), _Alignof(f32));
    p->cap = n;
    p->len = 0;
}

bool app_pos_push_row(app_pos_position *p, f32 vx, f32 vy) {
    if (p->len == p->cap) return false;
    p->x[p->len] = vx;
    p->y[p->len] = vy;
    p->len++;
    return true;
}




f32 app_pos_sum_x(app_pos_position *p) {
    keel_slice_f32 xs = keel_slice_f32_from(p->x, p->len);
    f32 total = 0;
    { keel_slice_f32 keel__c0 = xs; size_t keel__n0 = keel_slice_f32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { f32 v = keel_slice_f32_get(keel__c0, i); total += v; } }
    return total;
}
