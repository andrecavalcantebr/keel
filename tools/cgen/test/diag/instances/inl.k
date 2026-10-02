module inl type T;
pub modifier cell { T v; };
pub inline T read(cell *c) { return c->v; }
