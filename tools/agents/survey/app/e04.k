module app.e04;
pub int f(void) {
    int t = 0;
    foreach (size_t i : 0..4) { t += (int)i; }
    foreach (size_t i : 2..6) t += (int)i;
    return t;
}
