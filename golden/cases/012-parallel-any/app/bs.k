module app.bs;
import keel.buffer   as buffer   types;
import keel.slice    as slice    types;
import keel.parallel as parallel;

/* Search: `win` is the only verdict that counts for the policy, so `ok` is
   true only if someone found it (§4.8). Whoever observes the interrupt
   request just stops searching — the query is not an exit. */
pub bool found_any(buffer i32 *xs, i32 target, buffer size_t *where) {
    parallel search ANY (size_t w : 0..4; slice i32 part : xs; (target, where)) {
        foreach (i32 x, size_t i : part) {
            if (parallel.interrupted(search)) break;
            if (x == target) { buffer.set(where, w, i + 1); win; }
        }
    }
    return parallel.ok(search);
}
