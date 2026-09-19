/* grids_grid_3_f32.h — gerado de grids.k pelo cgen, perfil C11. */
#ifndef GRIDS_GRADE_3_F32_H
#define GRIDS_GRADE_3_F32_H
#include "grids_grid_3_f32.type.h"

#line 5 "grids.k"
static inline size_t grids_grid_3_f32_length(grids_grid_3_f32 *g);
#line 10 "grids.k"
static inline f32 *grids_grid_3_f32_ptr3(grids_grid_3_f32 *g, size_t idx[static 3]);

#line 5 "grids.k"
static inline size_t grids_grid_3_f32_length(grids_grid_3_f32 *g) {
    size_t n = 1;
    for (size_t d = 0; d < 3; d++) n *= g->dims[d];
    return n;
}
static inline f32 *grids_grid_3_f32_ptr3(grids_grid_3_f32 *g, size_t idx[static 3]) {
    size_t off = 0, step = 1;
    for (size_t d = 3; d-- > 0; ) { off += idx[d] * step; step *= g->dims[d]; }
    return g->ptr + off;
}
#endif