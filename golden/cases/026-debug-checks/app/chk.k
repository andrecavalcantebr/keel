module app.chk;
import keel.arena  as arena  types;
import keel.buffer as buffer types;
import keel.slice  as slice;
import_c <stdlib.h>;
import_c <stdint.h>;

/* One run takes one check by its number (golden DEBUG): each branch breaks a
   rule that only the debug build verifies (backend §5.17), and aborts there. */
pub int main(int argc, char **argv) {
    array i32 v[4] = {1, 2, 3, 4};
    buffer i32 b = buffer.of(v);
    size_t i = argc > 2 ? (size_t)atoi(argv[2]) : 4;
    int which = argc > 1 ? atoi(argv[1]) : 0;
    if (which == 1) return buffer.get(b, i);
    if (which == 2) { buffer.set(b, i, 0); return 0; }
    if (which == 3) { slice.slice i32 s = buffer.as_slice(b, 3, 1); return (int)slice.length(s); }
    if (which == 4) return v[i];
    if (which == 5) {
        array u8 raw[64];
        arena a;
        if (!arena.from_array(a, raw)) return 1;
        return arena.alloc(a, SIZE_MAX, 2, 1) != NULL;
    }
    return 0;
}
