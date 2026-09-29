module a;
import keel.buffer as buffer types;

constexpr size_t N = 4;

pub i32 f(void) {
    array i32 m[2,3];
    array i32 v[N];
    i32 a = m[1];                                /* DIAG: partial-array-index */
    i32 b = m[2,0];                              /* DIAG: array-index-above-dimension */
    i32 c = m[1][3];                             /* DIAG: array-index-above-dimension */
    i32 d = v[4];                                /* DIAG: array-index-above-dimension */
    i32 ok = m[1,2] + v[3] + v[N - 1] + m[0][0];
    return a + b + c + d + ok;
}

pub size_t g(buffer i32 b) {
    slice i32 s = b[5..2];                       /* DIAG: inverted-range-index */
    slice i32 t = b[2..5];
    slice i32 u = b[1][3..];                     /* DIAG: open-range-index-on-complex-path */
    return 0;
}

pub i32 h(array i32 w[8], array i32 z[], array i32 ok[size_t n], array i32 grid[2,3]) {  /* DIAG: array-1d-as-parameter */ /* DIAG: array-parameter-without-dimension */
    return 0;
}

pub size_t k(size_t n) {
    array i32 q[2,3];
    return keel.dim(q, 1) + keel.dim(q, n);      /* DIAG: nonconstant-dim-index */
}
