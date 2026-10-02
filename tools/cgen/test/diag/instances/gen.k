module gen type T;
pub modifier box { T v; };
pub inline T get(box *b) { return b->v; }
pub size_t put(box *b, T v);
