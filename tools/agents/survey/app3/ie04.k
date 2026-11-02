module app3.ie04;
pub extern_c {
    /* a comment inside extern_c */
    static inline int triple(int v) { return v * 3; }
}
pub int f(int v) { return triple(v); }
