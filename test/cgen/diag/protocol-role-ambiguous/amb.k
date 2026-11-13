module amb;
// two protocols that declare the same verb and arity collide at the implementer, never at a module
// that only calls through one of them (spec §5.1, rule 10)

pub protocol P1 type C { child i32 *val(parent C *c); }
pub protocol P2agree type C { child i32 *val(parent C *c); }
pub protocol P3 type C { child i32 *val2(parent C *c); }
pub protocol P4 type C { child i32 *val2(parent C *c); }
pub protocol P5 type C { i32 *val3(invalidates C *c); }
pub protocol P6 type C { child i32 *val3(parent C *c); }

pub typedef struct { i32 v; } T;

// |S| = 1 (only keel.array's IndexGet-shaped prelude verbs don't match "length" here, nothing to
// collide with "val"-named verbs outside this file): the usual case is untouched by this file at all.

// P1 and P2agree require exactly the same roles: writing them once covers the union, no ambiguity
pub inline child i32 *val(parent T *c) { return &c->v; }

// P3 and P4 also agree with each other, but |S| = 2 still asks for an explicit write: agreement between
// the colliding protocols does not make the verb exempt
pub inline i32 *val2(T *c) { return &c->v; }          /* DIAG: protocol-role-ambiguous */

// P5 and P6 disagree across axes (invalidates vs parent/child): nothing written leaves both gaps open
pub inline i32 *val3(T *c) { return &c->v; }          /* DIAG: protocol-role-ambiguous */
