module app.x04;
priv extern_c { static int cache = 3; }
pub int f(void) { return cache; }
