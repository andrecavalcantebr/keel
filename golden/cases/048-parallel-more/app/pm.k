module app.pm;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;

pub constexpr size_t NEED = 2;

pub bool named_policy(buffer i32 *xs) {
    parallel v NEED (size_t w : 0..4; slice.slice i32 part : xs) {
        foreach (i32 x, size_t i : part) { if (x < 0) win; }
    }
    return parallel.ok(v);
}

pub bool one_line(buffer i32 *xs) {
    parallel v ALL (size_t w : 0..4; slice.slice i32 part : xs) { foreach (i32 x, size_t i : part) { if (x < 0) fail; } }
    return parallel.failed(v);
}
