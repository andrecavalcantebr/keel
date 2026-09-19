/* grid_grade_3_f32.proto.h — gerado de grid.k pelo cgen, perfil C11. */
#ifndef GRID_GRADE_3_F32_PROTO_H
#define GRID_GRADE_3_F32_PROTO_H
#include "grid_grade_3_f32.type.h"

static inline size_t grid_grade_3_f32_length(grid_grade_3_f32 *g);
/* sufixo 3: o ponto de chamada escreve três índices, embora o C receba um
   argumento além do contêiner (backend §2.2) */
static inline f32 *grid_grade_3_f32_ptr3(grid_grade_3_f32 *g, size_t idx[static 3]);
#endif /* GRID_GRADE_3_F32_PROTO_H */
