/* arr.c — generated from arr.k by cgen, C23 profile. */
#include "arr.h"
#line 1 "arr.k"


size_t arr_bytes(void) {
    i32 v[4];
    f32 m[2][3];
    return sizeof(v) + sizeof(m);
}

size_t arr_count(i32 v[static 2][3]) {
    (void)v; return 3;
}
