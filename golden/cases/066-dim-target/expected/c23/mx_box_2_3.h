/* mx_box_2_3.h — generated from mx.k by cgen, C23 profile. */
#ifndef MX_BOX_2_3_H
#define MX_BOX_2_3_H

#include "mx_box_2_3.type.h"

#line 8 "mx.k"
static inline mx_box_2_3 mx_box_2_3_zeros(void);
#line 13 "mx.k"
static inline void mx_box_2_3_set(mx_box_2_3 *b, size_t i, size_t j, double x);
static inline double mx_box_2_3_get(mx_box_2_3 *b, size_t i, size_t j);

#line 8 "mx.k"
static inline mx_box_2_3 mx_box_2_3_zeros(void) {
    mx_box_2_3 v = {0};
    return v;
}
#line 13 "mx.k"
static inline void mx_box_2_3_set(mx_box_2_3 *b, size_t i, size_t j, double x) { b->data[i][j] = x; }
static inline double mx_box_2_3_get(mx_box_2_3 *b, size_t i, size_t j) { return b->data[i][j]; }

#endif /* MX_BOX_2_3_H */
