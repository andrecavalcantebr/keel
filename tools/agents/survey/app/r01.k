module app.r01;
import keel.array as array;
import keel.slice as slice;
pub size_t f(void) {
    array i32 v[6] = {1, 2, 3, 4, 5, 6};
    slice.slice i32 s = v[2..];
    return slice.length(s);
}
