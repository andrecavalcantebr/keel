/* app/app_mi.c — generated from app/mi.k by cgen, C11 profile. */
#include "app/app_mi.h"
#include "grids_grid_2_i32.h"
#line 1 "app/mi.k"




i32 app_mi_cell(grids_grid_2_i32 *g, size_t i, size_t j) { return *grids_grid_2_i32_ptr_2(g, (size_t[2]){i, j}); }
void app_mi_put(grids_grid_2_i32 *g, size_t i, size_t j, i32 v) { *grids_grid_2_i32_ptr_2(g, (size_t[2]){i, j}) = v; }

int app_mi_main(int argc, char **argv) {
    i32 data[6] = {0};
    grids_grid_2_i32 g;
    g.dims[0] = 2; g.dims[1] = 3; g.ptr = data;
    app_mi_put(&g, 1, 2, 7);
    if (data[5] != 7) return 1;
    if (app_mi_cell(&g, 1, 2) != 7) return 2;
    size_t r = 0;
    i32 w = *grids_grid_2_i32_ptr_2(&g, (size_t[2]){r++, 2});
    if (r != 1 || w != 0) return 3;
    *grids_grid_2_i32_ptr_2(&g, (size_t[2]){1, 0}) = 9;
    if (data[3] != 9) return 4;
    puts("ok");
    return 0;
}
