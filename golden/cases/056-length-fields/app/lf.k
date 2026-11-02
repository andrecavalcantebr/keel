module app.lf;
import keel.array as array;
import keel.slice as slice;

pub typedef struct { array i32 v[5]; } Box;

pub constexpr size_t K = 3;
priv array i32 tab[K];

pub size_t field_ptr(Box *b) {
    return keel.length(b->v);
}

pub size_t field_val(Box b) {
    return keel.length(b.v);
}

pub size_t mod_len(void) {
    return keel.length(tab);
}

pub size_t d(array i32 v[2, 3]) {
    return keel.dim(v, 1);
}

pub size_t field_slice(Box *b) {
    slice.slice i32 s = b->v[1..4];
    return slice.length(s);
}
