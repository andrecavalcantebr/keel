module app.ml;
import keel.buffer as buffer types;
import_c <stdio.h>;

pub typedef struct { int v; } R;

priv void release(R *r);

pub int use(R *r, buffer i32 *xs) {
    defer release(r);
    f32 *a = buffer.ptr(xs);            /* ERROR */
    i32 t = 0;
    foreach (i32 x, size_t i : xs) t += x;
    f32 *b = buffer.ptr(xs);            /* ERROR */
    (void)a; (void)b;
    return t;
}

priv void release(R *r) {
    f32 *c = (i32 *)0;                  /* ERROR */
    (void)c; r->v = 0;
}
