module idx;

pub i32 pick(size_t i, size_t j) {
    array i32 v[4];
    array i32 m[2,3];
    v[i] = 7;
    m[i,j] = v[i];
    return m[1][j] + v[2];
}
