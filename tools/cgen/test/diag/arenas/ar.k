module ar;
import keel.arena as arena types;
import keel.buffer as buffer types;

pub bool f(size_t n) {
    array i32 ints[4];
    array u8 bytes[64];
    array const i32 fixed[4] = {1, 2, 3, 4};
    arena b;
    if (!arena.from_array(b, bytes)) return false;
    buffer i32 x = buffer.of(fixed);                   /* DIAG: buffer-over-const */
    i32 *p = ints;
    buffer i32 y = buffer.of(p);                       /* DIAG: buffer-of-unknown-size */
    return true;
}
