module app3.ce03;
import keel.range as range types;
pub size_t f(void) {
    constexpr size_t N = 5;
    size_t t = 0;
    foreach (auto i : 0..N) t += i;
    return t;
}
