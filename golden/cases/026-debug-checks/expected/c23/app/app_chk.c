/* app/app_chk.c — generated from app/chk.k by cgen, C23 profile. */
#include "app/app_chk.h"
#include "keel/keel_arena.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/chk.k"









int app_chk_main(int argc, char **argv) {
    i32 v[4] = {1, 2, 3, 4};
    keel_buffer_i32 b = keel_buffer_i32_of(v, 4);
    size_t i = argc > 2 ? (size_t)atoi(argv[2]) : 4;
    int which = argc > 1 ? atoi(argv[1]) : 0;
    if (which == 1) return keel_buffer_i32_get(&b, i);
    if (which == 2) { keel_buffer_i32_set(&b, i, 0); return 0; }
    if (which == 3) { keel_slice_i32 s = keel_buffer_i32_as_slice2(&b, 3, 1); return (int)keel_slice_i32_length(s); }
    if (which == 4) return v[keel_index(i, 4)];
    if (which == 5) {
        u8 raw[64];
        keel_arena a = {0};
        if (!keel_arena_from_array(&a, raw, 64)) return 1;
        return keel_arena_alloc3(&a, SIZE_MAX, 2, 1) != NULL;
    }
    return 0;
}
