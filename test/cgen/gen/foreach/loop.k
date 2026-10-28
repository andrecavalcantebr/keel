module loop;

pub size_t sum(size_t n) {
    size_t t = 0;
    foreach (auto i : 0..n) t += i;
    foreach (auto j : 2..n) { t += j; }
    foreach (u32 k : 1..4) { t += k; }
    return t;
}
