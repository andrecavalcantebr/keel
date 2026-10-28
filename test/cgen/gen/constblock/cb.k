module cb;

pub size_t twice(void) {
    constexpr size_t N = 8;
    int buf[N];
    return sizeof(buf) / sizeof(int) + N;
}

pub size_t other(void) {
    { constexpr size_t N = 2; return N; }
}

pub size_t third(void) {
    constexpr size_t M = 2 + 3;
    return M;
}

pub constexpr int SPAN = 1 << 4;
