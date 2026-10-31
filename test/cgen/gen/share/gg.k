module gg type T;

pub constexpr i32 LIMIT = 8;

pub modifier box byref { T v; }

pub inline T get(box *b) { return b->v; }
