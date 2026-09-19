/* grid_grade_3_f32.h — gerado de grid.k pelo cgen, perfil C23. */
#ifndef GRID_GRADE_3_F32_H
#define GRID_GRADE_3_F32_H
#include "grid_grade_3_f32.type.h"

#line 5 "grid.k"
static inline size_t grid_grade_3_f32_length(grid_grade_3_f32 *g);
#line 10 "grid.k"
static inline f32 *grid_grade_3_f32_ptr3(grid_grade_3_f32 *g, size_t idx[static 3]);

#line 5 "grid.k"
static inline size_t grid_grade_3_f32_length(grid_grade_3_f32 *g) {
    size_t n = 1;
    for (size_t d = 0; d < 3; d++) n *= g->dims[d];
    return n;
}
static inline f32 *grid_grade_3_f32_ptr3(grid_grade_3_f32 *g, size_t idx[static 3]) {
    size_t off = 0, passo = 1;
    for (size_t d = 3; d-- > 0; ) { off += idx[d] * passo; passo *= g->dims[d]; }
    return g->ptr + off;
}
#endif /* GRID_GRADE_3_F32_H */