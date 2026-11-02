module app.al;
import keel.array as array;
import keel.slice as slice;

pub typedef struct { int n; } Box;
pub typedef struct { array i32 a[2], b[3]; } Pair;

pub size_t from_two(void) {
    array i32 v[6] = {1, 2, 3, 4, 5, 6};
    slice.slice i32 s = v[2..];
    slice.slice i32 t = v[..];
    return slice.length(s) + slice.length(t);
}

pub int pair(Pair *p) { return p->a[0] + p->b[1]; }

pub size_t cap(array i32 v[size_t n]) { return keel.capacity(v); }
pub size_t plain(array i32 v[2, 3]) { return keel.length(v); }
pub size_t boxes(void) {
    array Box bs[3];
    return keel.length(bs);
}
