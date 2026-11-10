/* mx_box_3_2.h — generated from mx.k by cgen, C11 profile. */
#ifndef MX_BOX_3_2_H
#define MX_BOX_3_2_H

#include "mx_box_3_2.type.h"
#include "mx_box_2_3.type.h"

#line 8 "mx.k"
static inline mx_box_2_3 mx_box_3_2_transpose(mx_box_3_2 *b);
#line 16 "mx.k"
static inline void mx_box_3_2_set(mx_box_3_2 *b, size_t i, size_t j, double x);
static inline double mx_box_3_2_get(mx_box_3_2 *b, size_t i, size_t j);

#line 8 "mx.k"
static inline mx_box_2_3 mx_box_3_2_transpose(mx_box_3_2 *b) {
    mx_box_2_3 t = {0};
    for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 2; ++j)
            t.data[j][i] = b->data[i][j];
    return t;
}
#line 16 "mx.k"
static inline void mx_box_3_2_set(mx_box_3_2 *b, size_t i, size_t j, double x) { b->data[i][j] = x; }
static inline double mx_box_3_2_get(mx_box_3_2 *b, size_t i, size_t j) { return b->data[i][j]; }

#endif /* MX_BOX_3_2_H */
