module app.pbox dim N type T;

pub constexpr size_t CAP = N;
pub typedef struct { T first; T last; } Pair;
pub modifier box byref { T v[N]; }

pub inline size_t cap(box *b) { return CAP; }
pub inline Pair ends(box *b) { Pair p = { b->v[0], b->v[N - 1] }; return p; }
