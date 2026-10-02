module i;
import keel.buffer as buffer types;
import keel.outcome as outcome types;
import keel.slice as slice types;

pub i32 g(buffer i32 b, outcome i32 o) {
    array i32 m[2,3];
    i32 a = b[1,2];                              /* DIAG: no-ptr-for-arity */
    i32 c = o[1];                                /* DIAG: no-ptr-for-arity */
    slice i32 t = slice.of(m);                   /* DIAG: flat-view-of-n-dim-array */
    slice i32 u = o[1..2];                       /* DIAG: protocol-not-satisfied */
    i32 ok = b[1] + m[1,2] + m[1][2];
    return a + c + ok;
}
