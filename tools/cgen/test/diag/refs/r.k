module r;
import keel.slice as slice types;

pub i32 f(i32 *origin) {
    i32 *ref p = origin;
    i32 *ref q;                                  /* DIAG: ref-without-initializer */
    i32 x = *p;
    p++;                                         /* DIAG: ref-arithmetic */
    i32 y = p[1];                                /* DIAG: ref-arithmetic */
    i32 z = *(p + 1);                            /* DIAG: ref-arithmetic */
    constexpr size_t K = 3;
    size_t *pk = &K;                             /* DIAG: constexpr-as-lvalue */
    K = 4;                                       /* DIAG: constexpr-as-lvalue */
    i32 one = 1;
    i32 *ref r = &one;
    slice i32 s = slice.from(i32, r, 1);         /* DIAG: slice-from-ref */
    return x + y + z + (i32)K;
}
