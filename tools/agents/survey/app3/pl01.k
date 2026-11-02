module app3.pl01;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;
pub constexpr size_t W = 3;
pub bool find(buffer i32 *xs, i32 target, buffer size_t *where) {
    parallel s ANY (size_t w : 0..W;
                    slice.slice i32 part : xs;
                    (target, where)) {
        foreach (i32 x, size_t i : part) {
            if (parallel.interrupted(s)) break;
            if (x == target) { buffer.set(where, w, i + 1); win; }
        }
    }
    return parallel.ok(s);
}
