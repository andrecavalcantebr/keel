module app.g;
import grids types;
import_c <stdio.h>;

constexpr i8 DIM = 3;

/* Both parameters are the SAME instance: the `dim` argument substitutes the
   value, never the spelling (§4.9). Were they distinct types, the C would not
   compile. */
pub f32 first(grid(3) f32 *a, grid(DIM) f32 *b) {
    return *ptr(a, (size_t[3]){0,0,0}) + *ptr(b, (size_t[3]){0,0,0});
}

pub int main(int argc, char **argv) {
    array f32 data[8];
    grid(DIM) f32 g;
    g.dims[0] = 2; g.dims[1] = 2; g.dims[2] = 2; g.ptr = data;
    data[0] = 1.5f;
    if (first(g, g) != 3.0f) return 1;      /* the same object in both positions */
    puts("ok");
    return 0;
}
