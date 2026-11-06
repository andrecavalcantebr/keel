module fv_use;
import fv types;

pub i32 sum3(i32 a, i32 b, i32 c) {
    i32 buf[3] = { a, b, c };
    vec i32 v = { 3, buf };
    i32 total = 0;
    foreach (i32 *p, size_t i : v) total += *p;
    return total;
}
