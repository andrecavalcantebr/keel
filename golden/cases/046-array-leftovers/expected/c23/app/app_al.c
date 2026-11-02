/* app/app_al.c — generated from app/al.k by cgen, C23 profile. */
#include "app/app_al.h"
#include "keel/keel_array_i32.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/al.k"







size_t app_al_from_two(void) {
    i32 v[6] = {1, 2, 3, 4, 5, 6};
    keel_slice_i32 s = keel_array_i32_as_slice2(v, 6, 2, keel_array_i32_length(v, 6));
    keel_slice_i32 t = keel_array_i32_as_slice2(v, 6, 0, keel_array_i32_length(v, 6));
    return keel_slice_i32_length(s) + keel_slice_i32_length(t);
}

int app_al_pair(app_al_Pair *p) { return p->a[0] + p->b[1]; }

size_t app_al_cap(i32 v[], size_t keel__n) { return keel__n; }
size_t app_al_plain(i32 v[static 2][3]) { return 2 * 3; }
size_t app_al_boxes(void) {
    app_al_Box bs[3];
    return sizeof(bs)/sizeof(app_al_Box);
}
