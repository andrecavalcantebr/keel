module app3.ce04;
pub constexpr size_t A = 2;
pub constexpr size_t B = A * A;
pub constexpr size_t C = B + A;
pub size_t c(void) { return C; }
