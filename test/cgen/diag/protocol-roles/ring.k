module ring;
// the roles of a protocol verb are inherited by the implementer that writes none (spec §5.1, rule 10)

pub typedef struct { i32 *at; size_t len; } Ring;
pub typedef struct { size_t i; } cursor;

pub inline child i32 *raw(parent Ring *r)    { return r->at; }
pub inline void clear(invalidates Ring *r)   { r->len = 0; }

// IndexPtr and Traversable: nothing is written, so `ptr` and `next` are
// `parent` on the ring and `child` on the result, and these bodies are covered
pub inline size_t length(Ring *r)            { return r->len; }
pub inline i32 *ptr(Ring *r, size_t i) {
    i32 *q = raw(r);
    return q;
}
pub inline cursor begin(Ring *r)             { cursor c = { 0 }; return c; }
pub inline bool has_next(Ring *r, cursor *c) { return c->i < r->len; }
pub inline i32 *next(Ring *r, cursor *c) {
    i32 *q = raw(r);
    return q;
}
