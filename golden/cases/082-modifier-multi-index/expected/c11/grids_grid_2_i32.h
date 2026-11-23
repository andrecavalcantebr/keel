/* grids_grid_2_i32.h — generated from grids.k by cgen, C11 profile. */
#ifndef GRIDS_GRID_2_I32_H
#define GRIDS_GRID_2_I32_H

#include "grids_grid_2_i32.type.h"

#line 5 "grids.k"
static inline size_t grids_grid_2_i32_length(grids_grid_2_i32 *g);
#line 10 "grids.k"
static inline i32 *grids_grid_2_i32_ptr_2(grids_grid_2_i32 *g, size_t idx[static 2]);

#line 5 "grids.k"
static inline size_t grids_grid_2_i32_length(grids_grid_2_i32 *g) {
    size_t n = 1;
    for (size_t d = 0; d < 2; d++) n *= g->dims[d];
    return n;
}
static inline i32 *grids_grid_2_i32_ptr_2(grids_grid_2_i32 *g, size_t idx[static 2]) {
    size_t off = 0, step = 1;
    for (size_t d = 2; d-- > 0; ) { off += idx[d] * step; step *= g->dims[d]; }
    return g->ptr + off;
}

#endif /* GRIDS_GRID_2_I32_H */
