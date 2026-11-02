module app.cl;

/* the line breaks of the INIT stay: in C23 the declaration is copied as written, and in C11
   the macro continues each line with `\` (backend §9.2, rule 1) */
pub constexpr size_t K = 1 +
    2;

pub size_t f(void) {
    constexpr size_t N = 3 +
        4;
    array char buf[N];
    return sizeof buf + K;
}
