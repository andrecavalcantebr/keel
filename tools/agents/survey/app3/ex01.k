module app3.ex01;
pub constexpr size_t R = 4;
pub extent struct tab [rows, R] {
    array i32 *v;
    size_t rows;
    size_t cap;
};
pub i32 at(struct tab *t, size_t i) { return t->v[i]; }
pub void set(struct tab *t, size_t i, i32 x) { t->v[i] = x; }
