module app.r09;
pub int f(void) {
    array i32 m[2, 3] = {{1, 2, 3}, {4, 5, 6}};
    return m[1][2];
}
