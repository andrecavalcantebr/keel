module dv;

pub typedef struct { int v; } Rec;

priv void reset(Rec *r) { r->v = 0; }
priv void mark_a(Rec *r) { r->v = r->v * 10 + 1; }
priv void mark_b(Rec *r) { r->v = r->v * 10 + 2; }

/* an early `return;` and the natural end: two exits, so a ladder, and with no
   value to carry there is no temporary. */
pub void early(Rec *r, int stop) {
    defer mark_a(r);
    if (stop) return;
    defer mark_b(r);
    r->v = r->v + 100;
}

/* the `return;` is the body of an `if` without braces, and the last statement
   is not a `return`: the natural end is the second exit. */
pub void guard(Rec *r, int stop) {
    defer reset(r);
    if (stop) return;
    r->v = 9;
}

/* a `return;` before any registration is not touched. */
pub void before(Rec *r, int stop) {
    if (stop) return;
    defer reset(r);
    r->v = 5;
}
