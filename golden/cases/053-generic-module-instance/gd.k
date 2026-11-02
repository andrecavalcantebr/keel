module gd dim N type T;

pub modifier box byref { T v[N]; }

pub constexpr size_t CAP = N;

pub typedef struct { T first; T last; } Pair;

pub inline size_t cap(box *b) { return CAP; }
