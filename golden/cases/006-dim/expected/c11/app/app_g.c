/* app/app_g.c — generated from app/g.k by cgen, C11 profile. */
#include "app/app_g.h"
#include "grids_grid_3_f32.h"
#line 1 "app/g.k"








f32 app_g_first(grids_grid_3_f32 *a, grids_grid_3_f32 *b) {
    return *grids_grid_3_f32_ptr3(a, (size_t[3]){0,0,0}) + *grids_grid_3_f32_ptr3(b, (size_t[3]){0,0,0});
}

int app_g_main(int argc, char **argv) {
    f32 data[8];
    grids_grid_3_f32 g;
    g.dims[0] = 2; g.dims[1] = 2; g.dims[2] = 2; g.ptr = data;
    data[0] = 1.5f;
    if (app_g_first(&g, &g) != 3.0f) return 1;
    puts("ok");
    return 0;
}
