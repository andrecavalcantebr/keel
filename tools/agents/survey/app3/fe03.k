module app3.fe03;
import keel.buffer as buffer;
pub size_t f(buffer.buffer i32 *xs, i32 stop) {
    size_t hits = 0;
    foreach (i32 x, size_t i : xs) {
        foreach (size_t j : 0..2) {
            if (x == stop) break;
            hits += j;
        }
    }
    return hits;
}
