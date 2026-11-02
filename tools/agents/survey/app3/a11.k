module app3.a11;
pub typedef struct { array i32 v[5]; } Box;
pub size_t f(Box *b) { return keel.length(b->v); }
