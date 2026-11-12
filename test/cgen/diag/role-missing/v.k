module v;
import keel.arena as arena types;
import keel.buffer as buffer types;
import_c <stdio.h>;

/* the body returns a child of its parameter, and the signature does not say so */
pub f32 *plain(arena *a) {
    f32 *p = arena.alloc(a, f32, 1);
    return p;                                     /* DIAG: role-missing */ /* DIAG: role-missing */
}

pub f32 *direct(arena *a) {
    return arena.alloc(a, f32, 1);                /* DIAG: role-missing */ /* DIAG: role-missing */
}

pub child f32 *covered(parent arena *a) {
    f32 *p = arena.alloc(a, f32, 1);
    return p;
}

pub child f32 *covered_direct(parent arena *a) {
    return arena.alloc(a, f32, 1);
}

/* a role that is left over is not reported */
pub child f32 *surplus(parent arena *a, f32 *q) {
    (void)a;
    return q;
}

/* the call is not the whole expression: the result is a bool */
pub bool is_null(arena *a) {
    return arena.alloc(a, f32, 1) == NULL;
}

/* the body invalidates a parameter */
pub void reset_plain(arena *a) {
    arena.reset(a);                               /* DIAG: role-missing */
}

pub void reset_covered(invalidates arena *a) {
    arena.reset(a);
}

pub void reset_consumes(consumes arena *a) {
    arena.reset(a);
}

/* an expression that mixes the symbol with operators is only suspect */
pub f32 *mixed(arena *a, bool c, f32 *q) {
    f32 *p = arena.alloc(a, f32, 1);
    return c ? p : q;                             /* DIAG: return-may-escape */
}

pub child f32 *mixed_covered(parent arena *a, bool c, f32 *q) {
    f32 *p = arena.alloc(a, f32, 1);
    return c ? p : q;
}

pub f32 first(arena *a) {
    f32 *p = arena.alloc(a, f32, 1);
    return p[0];
}

pub u8 *sliced(void) {
    array u8 local[8];
    return (u8 *)local;                           /* DIAG: return-may-escape */
}
