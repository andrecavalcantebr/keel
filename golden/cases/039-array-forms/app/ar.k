module app.ar;

pub typedef struct { array i32 v[4]; } Box;
pub typedef struct { array i32 m[2, 3]; } Grid;
priv array i32 g[4] = {5, 6, 7, 8};

pub int at_lit(Box *b) { return b->v[2]; }
pub int at_val(Box b) { return b.v[2]; }
pub void put(Box *b, int i) { b->v[i] = 1; }
pub int cell(Grid *gr, int i, int j) { return gr->m[i, j]; }
pub int cell_lit(Grid *gr) { return gr->m[1, 2]; }

pub size_t len_local(void) {
    array i32 x[2, 3];
    return keel.length(x) + keel.dim(x, 1) + keel.capacity(x);
}
pub size_t dim_mod(void) { return keel.dim(g, 0); }
pub size_t len_param(array i32 v[size_t n]) { return keel.length(v) + keel.dim(v, 0); }

pub int pair(void) {
    array i32 a[3], b[4];
    a[0] = 1; b[1] = 2;
    return a[0] + b[1];
}
