/* gen/app/g.c — perfil C11 */
#include "keel/prelude.h"
#include "grid_grade_3_f32.h"
#include <stdio.h>

#define app_g_DIM ((i8)3)
static const i8 app_g_DIM__chk = 3;


f32 app_g_primeiro(grid_grade_3_f32 *a, grid_grade_3_f32 *b) {
    return *grid_grade_3_f32_ptr3(a, (size_t[3]){0,0,0})
         + *grid_grade_3_f32_ptr3(b, (size_t[3]){0,0,0});
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    f32 app_g_dados[8];
    grid_grade_3_f32 g = {0};
    g.dims[0] = 2; g.dims[1] = 2; g.dims[2] = 2; g.ptr = app_g_dados;
    app_g_dados[0] = 1.5f;
    if (app_g_primeiro(&g, &g) != 3.0f) return 1;
    puts("ok");
    return 0;
}
