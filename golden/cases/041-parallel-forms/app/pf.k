module app.pf;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;

pub constexpr size_t W = 4;

pub bool two_lines(buffer i32 *xs) {
    parallel v ALL (size_t w : 0..4;
                    slice.slice i32 part : xs) {
        foreach (i32 x, size_t i : part) {
            if (x < 0) fail;
        }
    }
    return parallel.failed(v);
}

pub bool named(buffer i32 *xs) {
    parallel v ALL (size_t w : 0..W; slice.slice i32 part : xs) {
        foreach (i32 x, size_t i : part) {
            if (x < 0) fail;
        }
    }
    return parallel.failed(v);
}

pub bool brace_below(buffer i32 *xs) {
    parallel v ALL (size_t w : 0..4; slice.slice i32 part : xs)
    {
        foreach (i32 x, size_t i : part) {
            if (x < 0) fail;
        }
    }
    return parallel.failed(v);
}
