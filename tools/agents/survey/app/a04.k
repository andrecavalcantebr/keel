module app.a04;
pub typedef struct { array i32 v[2, 3]; } Box;
pub int f(Box *b) { return b->v[1, 2]; }
