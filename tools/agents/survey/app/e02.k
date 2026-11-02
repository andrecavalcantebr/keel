module app.e02;
import keel.range as range types;
pub size_t f(range *r) {
    size_t t = 0;
    foreach (size_t v : r) t += v;
    return t;
}
