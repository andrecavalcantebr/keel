module app3.a01;
pub typedef struct { array i32 a[2], b[3]; } Pair;
pub int f(Pair *p) { return p->a[1] + p->b[2]; }
