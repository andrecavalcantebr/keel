module app.af;
import keel.buffer as buffer;
import keel.slice as slice;
import keel.range as range types;

priv buffer.buffer i32 *pick(buffer.buffer i32 *a) { return a; }

pub size_t adapt(slice.slice i32 *sp, range *r) {
    return slice.length(sp) + range.first(r) + range.limit(r);
}

pub size_t over_range(range *r) {
    size_t t = 0;
    foreach (size_t v : r) t += v;
    return t;
}

pub int over_call(buffer.buffer i32 *xs) {
    int t = 0;
    foreach (i32 x, size_t i : pick(xs)) t += x;
    return t;
}
