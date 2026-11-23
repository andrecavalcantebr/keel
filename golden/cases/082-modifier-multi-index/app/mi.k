module app.mi;
import grids types;
import_c <stdio.h>;

pub i32 cell(grid(2) i32 *g, size_t i, size_t j) { return g[i, j]; }
pub void put(grid(2) i32 *g, size_t i, size_t j, i32 v) { g[i, j] = v; }

pub int main(int argc, char **argv) {
    i32 data[6] = {0};
    grid(2) i32 g;
    g.dims[0] = 2; g.dims[1] = 3; g.ptr = data;
    put(g, 1, 2, 7);
    if (data[5] != 7) return 1;
    if (cell(g, 1, 2) != 7) return 2;
    size_t r = 0;
    i32 w = g[r++, 2];
    if (r != 1 || w != 0) return 3;
    g[1, 0] = 9;
    if (data[3] != 9) return 4;
    puts("ok");
    return 0;
}
