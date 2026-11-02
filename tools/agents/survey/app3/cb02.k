module app3.cb02;
import keel.buffer as buffer;
pub extent struct cloud [len, cap] {
    array f32 *x;
    size_t len, cap;
};
pub f32 total(struct cloud *c, buffer.buffer f32 *w) {
    f32 t = 0;
    foreach (f32 v, size_t i : w) {
        if (i < c->len) t += v * c->x[i];
    }
    return t;
}
