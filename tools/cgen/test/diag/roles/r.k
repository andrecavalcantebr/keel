module r;
// the protocols come with the prelude (spec §5.1)

pub typedef struct { i32 *at; size_t len; } Ring;
pub typedef struct { size_t i; } cursor;

pub inline cursor begin(consumes Ring *r)      { cursor c = { 0 }; return c; }
pub inline bool   has_next(Ring *r, cursor *c) { return c->i < r->len; }
pub inline i32   *next(Ring *r, cursor *c)     { return &r->at[c->i++]; }

pub size_t count(Traversable C items) { return 0; }

pub size_t use(Ring *x) {
    return count(x);                             /* DIAG: protocol-role-mismatch */
}
