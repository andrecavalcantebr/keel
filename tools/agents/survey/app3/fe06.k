module app3.fe06;
priv int acc = 0;
priv void add(size_t i) { acc += (int)i; }
pub int f(size_t n) {
    acc = 0;
    foreach (size_t i : 0..n) add(i);
    return acc;
}
