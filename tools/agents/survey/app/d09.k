module app.d09;
priv extern_c {
    static int cache;
}
pub int f(void) { return cache; }
