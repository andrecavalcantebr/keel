module app.a03;
pub typedef struct { array i32 v[4]; } Box;
pub void f(Box *b, int i) { b->v[i] = 1; }
