/* app/app_packed.c — generated from app/packed.k by cgen, C23 profile. */
#include "app/app_packed.h"
#include "keel/keel_arena.h"
#include "keel/keel_bitbuffer_13_u16.h"
#include "keel/keel_bitbuffer_1_bool.h"
#include "keel/keel_bitbuffer_33_u64.h"
#include "keel/keel_bitbuffer_3_u8.h"
#include "keel/keel_bitbuffer_4_u8.h"
#include "keel/keel_bitbuffer_5_u8.h"
#include "keel/keel_bitslice_3_u8.h"
#include "keel/keel_bitslice_4_u8.h"
#include "keel/keel_bitslice_5_u8.h"
#include "keel/keel_outcome_keel_bitbuffer_5_u8.h"
#include "keel/keel_outcome_keel_bitslice_5_u8.h"
#include "keel/keel_outcome_u8.h"
#include "keel/keel_range.h"
#include "keel/keel_slice_of_keel_bitbuffer_3_u8.h"
#line 1 "app/packed.k"










void app_packed_layout(u8 *store) {
    keel_bitbuffer_3_u8 g = keel_bitbuffer_3_u8_from(store, 8);
    for (u8 i = 0; i < 8; ++i) keel_bitbuffer_3_u8_push(&g, i);
}



i32 app_packed_widths(keel_arena *a) {
    keel_bitbuffer_3_u8 g3 = keel_bitbuffer_3_u8_make(a, 100);
    for (u8 i = 0; i < 100; ++i) if (!keel_bitbuffer_3_u8_push(&g3, i % 8)) return 1;
    if (keel_bitbuffer_3_u8_push(&g3, 1)) return 2;
    for (size_t i = 0; i < 100; ++i) if (keel_bitbuffer_3_u8_get(&g3, i) != i % 8) return 3;
    if (keel_bitbuffer_3_u8_bytes(&g3) != 38) return 4;

    keel_bitbuffer_5_u8 g5 = keel_bitbuffer_5_u8_make(a, 33);
    for (u8 i = 0; i < 33; ++i) keel_bitbuffer_5_u8_push(&g5, (u8)(i * 7) % 32);
    for (u8 i = 0; i < 33; ++i) if (keel_bitbuffer_5_u8_get(&g5, i) != (u8)(i * 7) % 32) return 5;

    keel_bitbuffer_13_u16 g13 = keel_bitbuffer_13_u16_make(a, 9);
    for (u16 i = 0; i < 9; ++i) keel_bitbuffer_13_u16_push(&g13, 8191 - i * 997);
    for (u16 i = 0; i < 9; ++i) if (keel_bitbuffer_13_u16_get(&g13, i) != 8191 - i * 997) return 6;

    keel_bitbuffer_33_u64 g33 = keel_bitbuffer_33_u64_make(a, 10);
    for (u64 i = 0; i < 10; ++i) keel_bitbuffer_33_u64_push(&g33, (i << 29) | i);
    for (u64 i = 0; i < 10; ++i) if (keel_bitbuffer_33_u64_get(&g33, i) != ((i << 29) | i)) return 7;

    keel_bitbuffer_1_bool flags = keel_bitbuffer_1_bool_make(a, 20);
    for (int i = 0; i < 20; ++i) keel_bitbuffer_1_bool_push(&flags, i % 3 == 0);
    for (int i = 0; i < 20; ++i) if (keel_bitbuffer_1_bool_get(&flags, i) != (i % 3 == 0)) return 8;
    return 0;
}



i32 app_packed_views(keel_arena *a) {
    keel_bitbuffer_3_u8 g = keel_bitbuffer_3_u8_make(a, 64);
    for (u8 i = 0; i < 64; ++i) keel_bitbuffer_3_u8_push(&g, i % 8);
    keel_bitslice_3_u8 v = keel_bitbuffer_3_u8_as_slice2(&g, 5, 60);
    if (keel_bitslice_3_u8_length(v) != 55) return 1;
    for (size_t i = 0; i < keel_bitslice_3_u8_length(v); ++i) if (keel_bitslice_3_u8_get(v, i) != (i + 5) % 8) return 2;
    keel_bitslice_3_u8 w = keel_bitslice_3_u8_as_slice(v, 3, 20);
    for (size_t i = 0; i < keel_bitslice_3_u8_length(w); ++i) if (keel_bitslice_3_u8_get(w, i) != (i + 8) % 8) return 3;
    keel_bitslice_3_u8_set(w, 0, 7);
    if (keel_bitbuffer_3_u8_get(&g, 8) != 7 || keel_bitbuffer_3_u8_get(&g, 7) != 7 || keel_bitbuffer_3_u8_get(&g, 9) != 1) return 4;
    keel_bitslice_3_u8 r = keel_bitbuffer_3_u8_as_slice1(&g, keel_range_of(10, 14));
    if (keel_bitslice_3_u8_length(r) != 4 || keel_bitslice_3_u8_get(r, 3) != 5) return 5;
    keel_bitslice_3_u8 all = keel_slice_of_keel_bitbuffer_3_u8(&g);
    if (keel_bitslice_3_u8_length(all) != 64) return 6;
    return 0;
}


u32 app_packed_total(keel_arena *a) {
    keel_bitbuffer_4_u8 g = keel_bitbuffer_4_u8_make(a, 30);
    for (u8 i = 0; i < 30; ++i) keel_bitbuffer_4_u8_push(&g, i % 16);
    u32 sum = 0;
    { keel_bitbuffer_4_u8 *keel__c0 = &g; size_t keel__n0 = keel_bitbuffer_4_u8_length(keel__c0); for (size_t i = 0; i < keel__n0; i++) { u8 q = keel_bitbuffer_4_u8_get(keel__c0, i); sum += (u32)q * (u32)(i + 1); } }
    keel_bitslice_4_u8 v = keel_bitbuffer_4_u8_as_slice2(&g, 4, 10);
    { keel_bitslice_4_u8 keel__c1 = v; size_t keel__n1 = keel_bitslice_4_u8_length(keel__c1); for (size_t i = 0; i < keel__n1; i++) { u8 q = keel_bitslice_4_u8_get(keel__c1, i); sum += q; } }
    return sum;
}


i32 app_packed_stack(keel_arena *a) {
    keel_bitbuffer_5_u8 g = keel_bitbuffer_5_u8_make(a, 3);
    keel_bitbuffer_5_u8_push(&g, 20); keel_bitbuffer_5_u8_push(&g, 21); keel_bitbuffer_5_u8_push(&g, 22);
    keel_outcome_u8 top = keel_bitbuffer_5_u8_pop(&g);
    if (keel_outcome_u8_failed(top) || keel_outcome_u8_value(top) != 22 || keel_bitbuffer_5_u8_length(&g) != 2) return 1;
    keel_outcome_u8 hit = keel_bitbuffer_5_u8_at(&g, 1);
    if (keel_outcome_u8_failed(hit) || keel_outcome_u8_value(hit) != 21) return 2;
    if (!keel_outcome_u8_failed(keel_bitbuffer_5_u8_at(&g, 2))) return 3;
    keel_bitbuffer_5_u8_clear(&g);
    if (keel_bitbuffer_5_u8_length(&g) != 0 || keel_bitbuffer_5_u8_capacity(&g) != 3) return 4;
    if (!keel_outcome_u8_failed(keel_bitbuffer_5_u8_pop(&g))) return 5;
    keel_bitbuffer_5_u8 none = keel_bitbuffer_5_u8_from(NULL, 10);
    if (keel_bitbuffer_5_u8_capacity(&none) != 0 || keel_bitbuffer_5_u8_push(&none, 1)) return 6;
    return 0;
}


i32 app_packed_cloned(keel_arena *a) {
    keel_bitbuffer_5_u8 g = keel_bitbuffer_5_u8_make(a, 30);
    for (u8 i = 0; i < 30; ++i) keel_bitbuffer_5_u8_push(&g, i);
    keel_outcome_keel_bitbuffer_5_u8 c = keel_bitbuffer_5_u8_clone(a, &g); if (keel_outcome_keel_bitbuffer_5_u8_failed(c)) { return 1; }
    keel_bitbuffer_5_u8 d = keel_outcome_keel_bitbuffer_5_u8_value(c);
    for (u8 i = 0; i < 30; ++i) if (keel_bitbuffer_5_u8_get(&d, i) != i) return 2;
    keel_bitslice_5_u8 v = keel_bitbuffer_5_u8_as_slice2(&g, 3, 27);
    keel_outcome_keel_bitslice_5_u8 vc = keel_bitslice_5_u8_clone(a, v); if (keel_outcome_keel_bitslice_5_u8_failed(vc)) { return 3; }
    keel_bitslice_5_u8 e = keel_outcome_keel_bitslice_5_u8_value(vc);
    if (keel_bitslice_5_u8_length(e) != 24) return 4;
    for (size_t i = 0; i < 24; ++i) if (keel_bitslice_5_u8_get(e, i) != i + 3) return 5;
    return 0;
}
