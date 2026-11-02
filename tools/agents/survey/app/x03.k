module app.x03;
extern_c { static inline int one(void) { return 1; } }
pub int f(void) { return one(); }
