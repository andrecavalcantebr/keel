module app.k02;
pub size_t f(void) {
    constexpr size_t N = 3 + // four
        4;
    array char b[N];
    return sizeof b;
}
