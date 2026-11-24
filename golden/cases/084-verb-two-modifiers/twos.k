module twos type T;

pub modifier out byref { size_t n; T *data; }
pub modifier tgt { T *data; }

pub inline i32 run(tgt d, size_t n) { return d.data[n]; }
pub inline void fill(out *o, T v) { for (size_t i = 0; i < o->n; i++) o->data[i] = v; }
