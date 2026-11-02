module app3.a12;
pub int f(void) {
    constexpr size_t N = 2 + 1;
    array i32 v[N];
    v[N - 1] = 9;
    return v[2];
}
