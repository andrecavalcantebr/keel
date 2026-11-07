module app.ppv;
import keel.slice    as slice types;
import keel.parallel as parallel;

pub bool all_non_negative(i32 *xs, size_t n) {
    slice.slice i32 input = slice.from(i32, xs, n);
    parallel v ALL (size_t w : 0..2; slice.slice i32 part : input) {
        foreach (i32 x, size_t i : part) { if (x < 0) fail; }
    }
    return parallel.ok(v);
}
