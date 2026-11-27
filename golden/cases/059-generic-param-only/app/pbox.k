module app.pbox dim N type T;

pub constexpr size_t CAP = N;
pub typedef T Elem;
pub modifier box byref { T v[N]; }

pub inline size_t cap(box *b) { return CAP; }
pub inline Elem first(box *b) { return b->v[0]; }
pub inline Elem last(box *b) { return b->v[N - 1]; }
