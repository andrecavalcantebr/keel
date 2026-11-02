module app3.ex04;
pub extent struct grid [rows, rcap] [cols, ccap] {
    array i32 *v;
    size_t rows, cols, rcap, ccap;
};
pub i32 *cell(struct grid *g, size_t i, size_t j) {
    return &g->v[i, j];
}
pub void fill(struct grid *g, size_t r, size_t c) {
    g->rows = r; g->cols = c;
    for (size_t i = 0; i < r; i++)
        for (size_t j = 0; j < c; j++)
            g->v[i, j] = (i32)(i + j);
}
