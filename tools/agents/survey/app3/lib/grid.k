module app3.lib.grid dim N type T;

pub modifier cells byref { T v[N]; }

pub constexpr size_t SIDE = N;

pub inline size_t side(cells *c) { return SIDE; }
