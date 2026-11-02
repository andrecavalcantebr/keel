module app.e03;
import keel.buffer as buffer types;
priv buffer i32 *pick(buffer i32 *a) { return a; }
pub int f(buffer i32 *xs) {
    int t = 0;
    foreach (i32 x, size_t i : pick(xs)) t += x;
    return t;
}
