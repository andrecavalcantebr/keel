module b;
import keel.buffer as buffer types;
import keel.slice as slice types;
import keel.corot as corot types;

priv i32 sum(array i32 v[size_t n]) { i32 tmp[n]; return v[0] + tmp[0]; }   /* DIAG: binder-as-dimension */
priv i32 grid(array i32 g[size_t r, 3]) { return g[0, 0]; }

pub array i32 row[4];
pub array i32 wide[2, 5];
pub i32 use(i32 *p, buffer slice i32 *bs, buffer i32 *xs) {
    i32 s = sum(row) + sum(p);                    /* DIAG: binder-argument-not-array */
    s += grid(wide);                              /* DIAG: array-argument-wrong-dimension */
    foreach (slice i32 e, size_t i : bs) { }      /* DIAG: binder-copies-container */
    walk (i32 *x, slice.cursor c : xs) { }        /* DIAG: cursor-type-mismatch */
    foreach (size_t k : 0..) { }                  /* DIAG: open-range-outside-index */
    corot r = {0};
    corot.fault(r, 0);                            /* DIAG: invalid-fault-code */
    corot.fault(r, -2);                           /* DIAG: invalid-fault-code */
    corot.fault(r, 3);
    return s;
}
