/* app/app_ir.c — generated from app/ir.k by cgen, C23 profile. */
#include "app/app_ir.h"
#include "keel/keel_array_i32.h"
#include "keel/keel_range.h"
#include "keel/keel_slice_i32.h"
#include "keel/keel_slice_of_keel_array_i32.h"
#line 1 "app/ir.k"










int app_ir_main(int argc, char **argv) {
    i32 v[6] = {10, 20, 30, 40, 50, 60};
    keel_slice_i32 s = keel_slice_of2_keel_array_i32(v, 6, 0, 6);
    size_t sum = 0;
    { for (size_t i = 1; i < keel_inclusive(3); i++) { sum += i; } }
    { for (size_t i = 1; i < 3; i++) { sum += i * 10; } }
    { for (size_t i = 1; i < 3; i++) { sum += i * 100; } }
    if (sum != 6 + 30 + 300) return 1;
    keel_range r = keel_range_of(2, keel_inclusive(4));
    if (r.first != 2 || r.limit != 5) return 2;
    keel_slice_i32 a = keel_slice_i32_as_slice(s, 1, keel_inclusive(3));
    keel_slice_i32 b = keel_slice_i32_as_slice(s, 0, keel_inclusive(1));
    keel_slice_i32 c = keel_slice_i32_as_slice(s, 4, keel_slice_i32_length(s));
    keel_slice_i32 d = keel_slice_i32_as_slice(s, 4, keel_slice_i32_length(s));
    keel_slice_i32 e = keel_slice_i32_as_slice(s, 4, keel_slice_i32_length(s));
    keel_slice_i32 f = keel_slice_i32_as_slice(s, 0, 2);
    keel_slice_i32 g = keel_slice_i32_as_slice(s, 2, 5);
    keel_slice_i32 h = keel_array_i32_as_slice2(v, 6, 1, keel_inclusive(2));
    if (keel_slice_i32_length(a) != 3 || keel_slice_i32_length(b) != 2) return 3;
    if (keel_slice_i32_length(c) != 2 || keel_slice_i32_length(d) != 2 || keel_slice_i32_length(e) != 2) return 4;
    if (keel_slice_i32_length(f) != 2 || keel_slice_i32_length(g) != 3 || keel_slice_i32_length(h) != 2) return 5;
    size_t top = argc > 1 ? (size_t)-1 : 3;
    keel_slice_i32 t = keel_slice_i32_as_slice(s, 0, keel_inclusive(top));
    if (keel_slice_i32_length(t) != 4) return 6;
    puts("ok");
    return 0;
}
