module app.d08;
extern_c {
    static inline int twice(int x) { return x * 2; }
}
pub int f(int x) { return twice(x); }
