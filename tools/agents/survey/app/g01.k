// instance: app.g01.box(3) i32
module app.g01 dim N type T;
pub constexpr size_t CAP = N;
pub modifier box byref { T v[N]; }
pub inline size_t cap(box *b) { return CAP; }
