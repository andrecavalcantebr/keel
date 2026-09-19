/* app/app_ac.c — gerado de app/ac.k pelo cgen, perfil C11. */
#include "app/app_ac.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_slice_i32.h"
#line 1 "app/ac.k"







i32 app_ac_bump(keel_buffer_i32 *b, size_t i) {
    *keel_buffer_i32_ptr1(b, i) = *keel_buffer_i32_ptr1(b, i) + 1;
    return *keel_buffer_i32_ptr1(b, i);
}

i32 app_ac_once(keel_buffer_i32 *b, size_t *i) {
    *keel_buffer_i32_ptr1(b, (*i)++) = 9;
    return (i32)*i;
}


i32 app_ac_cube(void) {
    i32 v[2][3][4];
    v[1][2][3] = 7;
    return v[1][2][3];
}


size_t app_ac_cut(keel_buffer_i32 *b) {
    keel_slice_i32 mid = keel_buffer_i32_as_slice2(b, 2, 7);
    keel_slice_i32 tail  = keel_buffer_i32_as_slice2(b, 5, keel_buffer_i32_length(b));
    keel_slice_i32 head  = keel_buffer_i32_as_slice2(b, 0, 3);
    keel_slice_i32 all = keel_buffer_i32_as_slice(b);
    return keel_slice_i32_length(mid) + keel_slice_i32_length(tail)
         + keel_slice_i32_length(head)  + keel_slice_i32_length(all);
}
