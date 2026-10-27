module coll type T;
import keel.outcome as outcome types;
import keel.slice as slice;
pub modifier stack byref { size_t cap, len; T *ptr; };
pub inline stack stack grow(stack *s);       /* DIAG: circular-generic */
pub inline size_t visit(stack *s, T v, T *p) {
    T w = v;
    foreach (size_t i : w) { }               /* DIAG: protocol-on-parameter */
    match (v) { }                            /* DIAG: protocol-on-parameter */
    size_t n = slice.length(p);              /* DIAG: protocol-on-parameter */
    return n + s->len;
}
