module app.dr;
import_c <stdio.h>;

pub typedef struct { int v; } Rec;

priv void reset(Rec *r) { r->v = 0; }
priv void mark_a(Rec *r) { r->v = r->v * 10 + 1; }
priv void mark_b(Rec *r) { r->v = r->v * 10 + 2; }

/* `expr` is evaluated BEFORE the cleanup, and the returned value is its own —
   hence the temporary (§4.7). Here the cleanup zeroes the field the
   expression reads, so the order is observable: returning 0 would be the
   wrong order. */
pub int consume(Rec *r, int mode) {
    defer reset(r);
    if (mode == 0) return r->v;
    return r->v * 2;
}

/* Two exits and two registrations: reverse order, and the `return` before the
   second registration cleans up only the first. */
pub int two(Rec *r, int early) {
    defer mark_a(r);
    if (early) return -1;
    defer mark_b(r);
    return -2;
}

/* `void` return: the expression is still evaluated, with no temporary. */
pub void nothing(Rec *r) {
    defer reset(r);
    r->v = 9;
    return;
}
