module arr;

pub size_t bytes(void) {
    array i32 v[4];
    array f32 m[2,3];
    return sizeof(v) + sizeof(m);
}

pub size_t count(array i32 v[3]) {
    (void)v; return 3;
}
