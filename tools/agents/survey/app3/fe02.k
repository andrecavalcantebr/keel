module app3.fe02;
import keel.range as range types;
pub size_t f(size_t n) {
    range r = 0..n;
    size_t t = 0;
    foreach (auto v : r) t += v;
    return t;
}
