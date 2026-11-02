module app3.fe01;
import keel.slice as slice types;
pub i32 f(void) {
    array i32 v[3] = {1, 2, 3};
    i32 t = 0;
    foreach (i32 x, size_t i : v) t += x * (i32)i;
    return t;
}
