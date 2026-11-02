module app3.ex03;
pub extent struct cloud [len, cap] {
    array f32 *x;
    array f32 *y;
    size_t len, cap;
};
pub bool push(struct cloud *c, f32 a, f32 b) {
    if (c->len == c->cap) return false;
    size_t i = c->len++;
    c->x[i] = a;
    c->y[i] = b;
    return true;
}
