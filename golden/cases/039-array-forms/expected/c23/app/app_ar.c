/* app/app_ar.c — generated from app/ar.k by cgen, C23 profile. */
#include "app/app_ar.h"
#line 1 "app/ar.k"




static i32 app_ar_g[4] = {5, 6, 7, 8};

int app_ar_at_lit(app_ar_Box *b) { return b->v[2]; }
int app_ar_at_val(app_ar_Box b) { return b.v[2]; }
void app_ar_put(app_ar_Box *b, int i) { b->v[keel_index(i, 4)] = 1; }
int app_ar_cell(app_ar_Grid *gr, int i, int j) { return gr->m[keel_index(i, 2)][keel_index(j, 3)]; }
int app_ar_cell_lit(app_ar_Grid *gr) { return gr->m[1][2]; }

size_t app_ar_len_local(void) {
    i32 x[2][3];
    return sizeof(x)/sizeof(i32) + 3 + sizeof(x)/sizeof(i32);
}
size_t app_ar_dim_mod(void) { return 4; }
size_t app_ar_len_param(i32 v[], size_t keel__n) { return keel__n + keel__n; }

int app_ar_pair(void) {
    i32 a[3], b[4];
    a[0] = 1; b[1] = 2;
    return a[0] + b[1];
}
