module app.r03;
pub typedef struct { array i32 a[2], b[3]; } Pair;
pub int f(Pair *p) { return p->a[0] + p->b[1]; }
