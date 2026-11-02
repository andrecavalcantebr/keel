module app3.a09;
import keel.array as array;
import keel.slice as slice types;
pub size_t f(array i32 v[size_t n]) {
    slice i32 s = v[..];
    return slice.length(s) + n;
}
