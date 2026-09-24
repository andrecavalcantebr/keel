module app.pos;

import keel.arena as arena types;
import keel.slice as slice types;

pub constexpr size_t H = 2;
pub constexpr size_t W = 3;

/* extent (spec §4.11): `x`/`y` marked `array` are columns under one control,
   `[len, cap]`; `active` is an ordinary field. By pointer, as written. */
pub extent struct position [len, cap] {
    array f32 *x;
    array f32 *y;
    bool active;
    size_t len, cap;
};

/* two dimensions by pointer: the stride is the inner capacity, `ccap`. */
pub extent struct grid [rows, rcap] [cols, ccap] {
    array i32 *v;
    size_t rows, cols, rcap, ccap;
};

/* embedded columns: each dimension is, by name, the group's capacity. */
pub extent struct image [h, H] [w, W] {
    array u8 r[H, W];
    array u8 g[H, W];
    size_t h, w;
};

pub bool reserve(struct position *p, arena *a, size_t n) {
    p->x = arena.alloc(a, f32, n);
    p->y = arena.alloc(a, f32, n);
    p->len = 0;
    p->cap = (p->x && p->y) ? n : 0;
    return p->cap > 0;
}

/* a row is reachable only below `len`: count first, then write. */
pub bool push_row(struct position *p, f32 vx, f32 vy) {
    if (p->len == p->cap) return false;
    size_t i = p->len++;
    p->x[i] = vx;
    p->y[i] = vy;
    return true;
}

/* the bare column is the C field: `slice.from` turns it into a slice. */
pub f32 sum_x(struct position *p) {
    slice f32 xs = slice.from(f32, p->x, p->len);
    f32 total = 0;
    foreach (f32 v, size_t i : xs) total += v;
    return total;
}

pub bool grid_init(struct grid *g, arena *a, size_t rcap, size_t ccap) {
    g->v = arena.alloc(a, i32, rcap * ccap);
    g->rows = 0;
    g->cols = 0;
    g->rcap = g->v ? rcap : 0;
    g->ccap = g->v ? ccap : 0;
    return g->v != NULL;
}

pub void grid_fill(struct grid *g, size_t rows, size_t cols) {
    g->rows = rows;
    g->cols = cols;
    for (size_t i = 0; i < rows; i++)
        for (size_t j = 0; j < cols; j++)
            g->v[i, j] = (i32)(i * 10 + j);
}

pub i32 grid_at(struct grid *g, size_t i, size_t j) {
    return g->v[i, j];
}

pub void paint(struct image *img) {
    img->h = H;
    img->w = W;
    for (size_t i = 0; i < H; i++)
        for (size_t j = 0; j < W; j++) {
            img->r[i, j] = (u8)(i * W + j);
            img->g[i, j] = (u8)(255 - img->r[i, j]);
        }
}
