/* app/app_g.c — gerado de app/g.k pelo cgen, perfil C11. */
#include "app/app_g.h"
#include "grid_grade_3_f32.h"
#line 9 "app/g.k"
f32 app_g_primeiro(grid_grade_3_f32 *a, grid_grade_3_f32 *b) {
    return *grid_grade_3_f32_ptr3(a, (size_t[3]){0,0,0}) + *grid_grade_3_f32_ptr3(b, (size_t[3]){0,0,0});
}

#line 13 "app/g.k"
int app_g_main(int argc, char **argv) {
    f32 dados[8];
    grid_grade_3_f32 g;
    g.dims[0] = 2; g.dims[1] = 2; g.dims[2] = 2; g.ptr = dados;
    dados[0] = 1.5f;
    if (app_g_primeiro(&g, &g) != 3.0f) return 1;      /* o mesmo objeto nas duas posições */
    puts("ok");
    return 0;
}
