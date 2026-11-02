/* app/app_lf.c — generated from app/lf.k by cgen, C11 profile. */
#include "app/app_lf.h"
#include "keel/keel_array_i32.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/lf.k"







static i32 app_lf_tab[app_lf_K];

size_t app_lf_field_ptr(app_lf_Box *b) {
    return sizeof(b->v)/sizeof(i32);
}

size_t app_lf_field_val(app_lf_Box b) {
    return sizeof(b.v)/sizeof(i32);
}

size_t app_lf_mod_len(void) {
    return sizeof(app_lf_tab)/sizeof(i32);
}

size_t app_lf_d(i32 v[static 2][3]) {
    return 3;
}

size_t app_lf_field_slice(app_lf_Box *b) {
    keel_slice_i32 s = keel_array_i32_as_slice2(b->v, 5, 1, 4);
    return keel_slice_i32_length(s);
}
