module app.ac;
import keel.buffer as buffer types;
import keel.slice  as slice  types;
import_c <stdio.h>;

/* `x[i]` is `*ptr(x,i)` — an lvalue, and the lowering is by function, never by
   macro: that is what guarantees `x[i++]` increments `i` exactly once (§5.3). */
pub i32 bump(buffer i32 *b, size_t i) {
    b[i] = b[i] + 1;
    return b[i];
}

pub i32 once(buffer i32 *b, size_t *i) {
    b[(*i)++] = 9;              /* the index is evaluated exactly once */
    return (i32)*i;
}

/* Multidimensional `array`: the comma form becomes brackets, with no function. */
pub i32 cube(void) {
    array i32 v[2,3,4];
    v[1,2,3] = 7;
    return v[1][2][3];
}

/* Range index: rvalue, half-open. The four forms of §4.6. */
pub size_t cut(buffer i32 *b) {
    slice i32 mid = b[2..7];
    slice i32 tail  = b[5..];
    slice i32 head  = b[..3];
    slice i32 all = b[..];
    return slice.length(mid) + slice.length(tail)
         + slice.length(head)  + slice.length(all);
}
