module grid dim N type T;

pub modifier grade byref { size_t dims[N]; T *ptr; }

pub inline size_t length(grade *g) {
    size_t n = 1;
    for (size_t d = 0; d < N; d++) n *= g->dims[d];
    return n;
}
pub inline T *ptr(grade *g, size_t idx[static N]) {
    size_t off = 0, passo = 1;
    for (size_t d = N; d-- > 0; ) { off += idx[d] * passo; passo *= g->dims[d]; }
    return g->ptr + off;
}
