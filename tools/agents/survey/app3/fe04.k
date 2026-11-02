module app3.fe04;
import keel.tagged as tagged types;
import keel.buffer as buffer;
pub tags Mode [FAST, SLOW];
pub i32 f(tagged Mode void m, buffer.buffer i32 *xs) {
    i32 t = 0;
    match (m) {
        FAST:
            foreach (i32 x, size_t i : xs) t += x;
            break;
        SLOW:
            t = 0;
    }
    return t;
}
