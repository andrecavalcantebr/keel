module ring;

pub typedef struct { i32 *at; size_t len; } Ring;

pub inline child i32 *raw(parent Ring *r)  { return r->at; }
pub inline void drop(consumes Ring *r)     { r->len = 0; }
pub inline void clear(invalidates Ring *r) { r->len = 0; }
pub inline void touch(Ring *r)             { r->len = 1; }
