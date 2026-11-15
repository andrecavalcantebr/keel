module use;
import keel types;
import c.m as mm types;
pub int run(void) {
    m u32 none; (void)none;       /* DIAG: modifier-dimension-count */
    m(4, 8) u32 two; (void)two;   /* DIAG: modifier-dimension-count */
    m(4) u32 ok; (void)ok;
    return 0;
}
