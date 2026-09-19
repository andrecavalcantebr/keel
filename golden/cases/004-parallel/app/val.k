module app.val;
import keel.buffer   as buffer   types;
import keel.slice    as slice    types;
import keel.parallel as parallel;

/* ALL policy: there is no flag, every worker runs to the end, and `failed`
   is deterministic (§4.8). That is what makes this case runnable.
   `parallel` does not traverse: it hands over the part, and traversing is
   the body's job. */
pub bool has_negative(buffer i32 *xs) {
    parallel validate ALL (size_t w : 0..4; slice i32 part : xs) {
        foreach (i32 x, size_t i : part) {
            if (x < 0) fail;
        }
    }
    return parallel.failed(validate);
}
