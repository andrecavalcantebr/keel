module app.q03;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;
pub bool f(buffer i32 *xs) {
    i32 lim = 0;
    parallel v ALL (size_t w : 0..4; slice.slice i32 part : xs; (lim)) {
        foreach (i32 x, size_t i : part) { if (x < lim) fail; }
    }
    return parallel.failed(v);
}
