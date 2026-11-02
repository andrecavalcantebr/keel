module app.x01;
pub extern_c {
    static inline int twice(int x) { return x * 2; }
}
pub int f(int x) { return twice(x); }
