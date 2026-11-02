module app.ef;

pub extern_c {
    static inline int twice(int x) { return x * 2; }
}

extern_c { static inline int one(void) { return 1; } }

extern_c [type_h] { typedef int T; }

priv extern_c { static int cache = 3; }

pub constexpr size_t K = 1 + /* two */
    2;

pub int f(T *t) { return twice(*t) + one() + cache + (int)K; }
pub size_t g(void) {
    constexpr size_t N = 3 + // four
        4;
    array char b[N];
    return sizeof b;
}
