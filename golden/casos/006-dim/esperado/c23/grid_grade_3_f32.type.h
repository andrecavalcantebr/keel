/* grid_grade_3_f32.type.h — instância de `grade(3) f32`.
   `grade(3) f32` e `grade(DIM) f32` com DIM==3 pedem ESTE arquivo, com este
   nome e este conteúdo: o nome canônico carrega o VALOR, e não a grafia
   (linguagem §4.9, backend §2.2). */
#ifndef GRID_GRADE_3_F32_TYPE_H
#define GRID_GRADE_3_F32_TYPE_H
#include "keel.type.h"

typedef struct grid_grade_3_f32 { size_t dims[3]; f32 *ptr; } grid_grade_3_f32;
#endif /* GRID_GRADE_3_F32_TYPE_H */
