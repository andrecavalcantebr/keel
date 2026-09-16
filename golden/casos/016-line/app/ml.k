module app.ml;
import keel.buffer as buffer types;
import_c <stdio.h>;

pub typedef struct { int v; } R;

priv void solta(R *r);

pub int usa(R *r, buffer i32 *xs) {
    defer solta(r);
    f32 *a = buffer.ptr(xs);            /* ERRO */
    i32 t = 0;
    foreach (i32 x, size_t i : xs) t += x;
    f32 *b = buffer.ptr(xs);            /* ERRO */
    (void)a; (void)b;
    return t;
}

priv void solta(R *r) {
    f32 *c = (i32 *)0;                  /* ERRO */
    (void)c; r->v = 0;
}
