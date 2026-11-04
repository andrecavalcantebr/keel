module app.peek type T;
pub modifier box byref { T v; }
priv inline T peek(box *b) { return b->v; }
pub inline T get(box *b) { return peek(b); }
