module use;
import keel types;
import c.m types;
pub int run(grid(2) i32 *g) {
    int a = g[1, 2, 3];           /* DIAG: no-ptr-for-arity */
    int b = g[1, 2];
    return a + b;
}
