module app.q01;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;
pub constexpr size_t T = 2;
pub bool f(buffer i32 *xs) {
    parallel v T (size_t w : 0..4; slice.slice i32 part : xs) {
        foreach (i32 x, size_t i : part) { if (x < 0) win; }
    }
    return parallel.ok(v);
}
