module app3.ex02;
pub extent struct vec [n, cap] {
    array f32 *x;
    size_t n, cap;
};
pub f32 sum(struct vec *v) {
    f32 t = 0;
    for (size_t i = 0; i < v->n; i++) t += v->x[i];
    return t;
}
