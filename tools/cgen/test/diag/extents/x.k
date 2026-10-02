module x;

constexpr size_t N = 8;
constexpr size_t M = 4;

pub extent struct ok [len, N] {
    array f32 a[N];
    size_t len;
};
pub extent struct grid [rows, rcap] [cols, ccap] {
    array i32 *v;
    size_t rows, cols, rcap, ccap;
};
pub extent struct nocount [n, N] {          /* DIAG: extent-count-not-field */
    array f32 a[N];
    size_t len;
};
pub extent struct nocap [len, Q] {          /* DIAG: extent-unknown-capacity */
    array f32 *a;
    size_t len;
};
pub extent struct nocol [len, cap] {        /* DIAG: extent-without-column */
    f32 *a;
    size_t len, cap;
};
pub extent struct mixed [len, N] {          /* DIAG: extent-mixed-columns */
    array f32 a[N];
    array f32 *b;
    size_t len;
};
pub extent struct fieldcap [len, cap] {
    array f32 a[cap];                       /* DIAG: extent-embedded-field-capacity */
    size_t len, cap;
};
pub extent struct dims [h, N] [w, M] {
    array u8 r[N, M];
    array u8 g[M, N];                       /* DIAG: extent-dimension-mismatch */
    array u8 b[N];                          /* DIAG: extent-dimension-mismatch */
    size_t h, w;
};

pub struct grid *get(void);

pub void use(struct ok *o, struct grid *g, struct ok s) {
    o->a[1] = 0;
    s.a[2] = 0;
    o->a[8] = 0;                            /* DIAG: extent-index-above-capacity */
    g->v[1, 2] = 0;
    g->v[1] = 0;                            /* DIAG: extent-index-arity */
    get()->v[1, 2] = 0;                     /* DIAG: extent-path-with-call */
}
