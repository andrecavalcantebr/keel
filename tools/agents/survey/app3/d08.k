module app3.d08;
pub int f(void) {
    constexpr int N = 2;
    int a[N], b[N + 1];
    a[0] = 1; b[2] = 4;
    return a[0] + b[2];
}
