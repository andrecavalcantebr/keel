/* proof.c — harness. The three functions of pf.k are the same search written three ways:
   a header over two lines, the workers counted by a constexpr, and the brace on a line of its
   own. Each must see a negative element, and none when there is none. */

#include "keel.type.h"
#include "app/app_pf.h"
#include "keel/keel_buffer_i32.h"

static int check(bool (*f)(keel_buffer_i32 *)) {
    i32 v[16];
    keel_buffer_i32 b = keel_buffer_i32_of(v, 16);
    for (size_t i = 0; i < 16; i++) keel_buffer_i32_set(&b, i, 1);
    if (f(&b)) return 1;
    keel_buffer_i32_set(&b, 13, -7);
    if (!f(&b)) return 2;
    return 0;
}

int main(void) {
    if (app_pf_W != 4) return 10;
    if (check(app_pf_two_lines)) return 11;
    if (check(app_pf_named)) return 12;
    if (check(app_pf_brace_below)) return 13;
    return 0;
}
