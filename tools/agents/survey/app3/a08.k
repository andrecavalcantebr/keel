module app3.a08;
import keel.array as array;
import keel.slice as slice types;
pub typedef struct { array i32 v[6]; } Vec;
pub size_t f(Vec *v) {
    slice i32 s = v->v[1..4];
    return slice.length(s);
}
