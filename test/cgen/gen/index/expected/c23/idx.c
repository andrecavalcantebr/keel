/* idx.c — generated from idx.k by cgen, C23 profile. */
#include "idx.h"
#line 1 "idx.k"


i32 idx_pick(size_t i, size_t j) {
    i32 v[4];
    i32 m[2][3];
    v[keel_index(i, 4)] = 7;
    m[keel_index(i, 2)][keel_index(j, 3)] = v[keel_index(i, 4)];
    return m[1][keel_index(j, 3)] + v[2];
}
