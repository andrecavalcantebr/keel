module app.align;
import keel.arena as arena types;
import_c <stdio.h>;
import_c <stdint.h>;

pub typedef struct { alignas(64) f64 x[8]; } Vec8;

/* No `alignas` on the backing array, and the base deliberately skewed:
   the allocation must come out aligned all the same (§4.4). */
pub int main(int argc, char **argv) {
    array u8 raw[8192];
    for (size_t d = 0; d < 8; d++) {
        arena a;
        if (!arena.from_array(a, raw + d, sizeof raw - 8)) return 1;
        i32  *p = arena.alloc(a, i32,  3);
        f64  *q = arena.alloc(a, f64,  2);
        Vec8 *v = arena.alloc(a, Vec8, 1);
        if (!p || !q || !v) return 2;
        if ((uintptr_t)p % alignof(i32) || (uintptr_t)q % alignof(f64)
            || (uintptr_t)v % 64) return 3;
    }
    puts("ok");
    return 0;
}
