module c.m type T;
pub modifier m byref { T *v; size_t len; }
pub inline bool ok(m *s) { return s->len > 0; }
