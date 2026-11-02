module app.rg;
import keel.array as array;
import keel.slice as slice;

priv array i32 p[3] = {1, 2, 3}, q[2] = {4, 5};

pub size_t window(void) {
    array i32 v[6] = {1, 2, 3, 4, 5, 6};
    slice.slice i32 s = v[1..4];
    slice.slice i32 t = v[..3];
    return slice.length(s) + slice.length(t) + keel.length(p) + keel.length(q);
}
