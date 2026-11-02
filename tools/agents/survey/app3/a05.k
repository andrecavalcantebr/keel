module app3.a05;
pub size_t f(array i32 m[size_t r, 4]) {
    size_t t = 0;
    for (size_t i = 0; i < r; i++) t += (size_t)m[i, 3];
    return t;
}
