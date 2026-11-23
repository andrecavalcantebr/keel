/* app/app_mi.h — generated from app/mi.k by cgen, C11 profile. */
#ifndef APP_APP_MI_H
#define APP_APP_MI_H

#include "app/app_mi.type.h"

typedef struct grids_grid_2_i32 grids_grid_2_i32;
#line 5 "app/mi.k"
i32 app_mi_cell(grids_grid_2_i32 *g, size_t i, size_t j);
void app_mi_put(grids_grid_2_i32 *g, size_t i, size_t j, i32 v);
#line 8 "app/mi.k"
int app_mi_main(int argc, char **argv);

#endif /* APP_APP_MI_H */
