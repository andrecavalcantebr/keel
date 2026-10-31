module scan;
import keel.buffer   as buffer   types;
import keel.slice    as slice;
import keel.parallel as parallel;

pub void sum(buffer i32 *xs, buffer u32 *totals) {
    parallel sum ALL (size_t w : 0..4; slice.slice i32 part : xs; (totals)) {
        u32 t = 0;
        walk (i32 *p, slice.cursor c : part) {
            t += (u32)*p;
        }
        totals[w] = t;
    }
}
