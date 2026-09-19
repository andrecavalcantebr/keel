/* app/app_align.c — generated from app/align.k by cgen, C23 profile. */
#include "app/app_align.h"
#include "keel/keel_arena.h"
#line 1 "app/align.k"









int app_align_main(int argc, char **argv) {
    u8 raw[8192];
    for (size_t d = 0; d < 8; d++) {
        keel_arena a = {0};
        if (!keel_arena_from_array(&a, raw + d, sizeof raw - 8)) return 1;
        i32  *p = (i32 *)keel_arena_alloc_n(&a, 3, sizeof(i32), alignof(i32));
        f64  *q = (f64 *)keel_arena_alloc_n(&a, 2, sizeof(f64), alignof(f64));
        app_align_Vec8 *v = (app_align_Vec8 *)keel_arena_alloc_n(&a, 1, sizeof(app_align_Vec8), alignof(app_align_Vec8));
        if (!p || !q || !v) return 2;
        if ((uintptr_t)p % alignof(i32) || (uintptr_t)q % alignof(f64)
            || (uintptr_t)v % 64) return 3;
    }
    puts("ok");
    return 0;
}
