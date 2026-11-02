module app3.pl03;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;
pub bool two(buffer i32 *xs) {
    parallel v ALL (size_t w : 0..4; slice.slice i32 part : xs)
    {
        foreach (i32 x, size_t i : part) { if (x < 0) fail; }
    }
    if (parallel.failed(v)) return false;
    return true;
}
