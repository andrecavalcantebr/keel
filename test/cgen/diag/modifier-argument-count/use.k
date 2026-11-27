module use;
import keel types;
import c.m as mm types;
pub bool by_pointer(m *t) { return t != NULL; }   /* DIAG: modifier-argument-count */
pub int run(void) {
    m none; (void)none;                           /* DIAG: modifier-argument-count */
    m u32 ok; (void)ok;
    return 0;
}
