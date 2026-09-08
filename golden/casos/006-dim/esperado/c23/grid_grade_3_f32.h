/* gen/grid_grade_3_f32.h — instância de `grade(3) f32`.
   `grade(3) f32` e `grade(DIM) f32` com DIM==3 pedem ESTE arquivo, com este
   nome e este conteúdo: o nome canônico carrega o VALOR, e não a grafia
   (linguagem §4.9, backend §2.2). */
#ifndef GRID_GRADE_3_F32_H
#define GRID_GRADE_3_F32_H
#include "grid.h"

typedef struct grid_grade_3_f32 { size_t dims[3]; f32 *ptr; } grid_grade_3_f32;

static inline size_t grid_grade_3_f32_length(grid_grade_3_f32 *g) {
    size_t n = 1;
    for (size_t d = 0; d < 3; d++) n *= g->dims[d];
    return n;
}
/* sufixo 3: o ponto de chamada escreve três índices, embora o C receba um
   argumento além do contêiner (backend §2.2) */
static inline f32 *grid_grade_3_f32_ptr3(grid_grade_3_f32 *g, size_t idx[static 3]) {
    size_t off = 0, passo = 1;
    for (size_t d = 3; d-- > 0; ) { off += idx[d] * passo; passo *= g->dims[d]; }
    return g->ptr + off;
}
#endif
