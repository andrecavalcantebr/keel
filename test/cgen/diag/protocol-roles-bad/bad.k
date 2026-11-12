module bad;
// the roles of a protocol verb reach the body of the implementer (spec §5.1, rule 10)

pub typedef struct { i32 *at; size_t len; } Ring;
pub typedef struct { size_t i; } cursor;

pub inline child i32 *raw(parent Ring *r) { return r->at; }

// `ptr` and `next` write no role: the protocol's `parent` and `child` cover the body
pub inline i32 *ptr(Ring *r, size_t i) {
    i32 *q = raw(r);
    return q;
}
pub inline i32 *next(Ring *r, cursor *c) {
    i32 *q = raw(r);
    return q;
}

// the same body in a function no protocol names: the roles are missing
pub inline i32 *peek(Ring *r) {
    i32 *q = raw(r);
    return q;                                     /* DIAG: role-missing */ /* DIAG: role-missing */
}

// `ptr` with another arity is no verb of the protocol
pub inline i32 *ptr3(Ring *r, size_t i, size_t j) {
    i32 *q = raw(r);
    return q;                                     /* DIAG: role-missing */ /* DIAG: role-missing */
}

// an expression that only mixes the symbol with operators is covered the same way
pub inline i32 *ptr_mixed(Ring *r, bool c, i32 *other) {
    i32 *q = raw(r);
    return c ? q : other;                         /* DIAG: return-may-escape */
}
