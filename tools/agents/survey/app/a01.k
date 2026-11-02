module app.a01;
pub typedef struct { array i32 v[4]; } Box;
pub int f(Box *b) { return b->v[2]; }
