/* proof.c — harness. The explicit `pub extern_c`, the three one-line extern_c forms, and the
   comments inside a constexpr initializer that leave the generated text. */

#include "keel.type.h"
#include "app/app_ef.h"

int main(void) {
    T t = 5;
    if (app_ef_f(&t) != 17) return 1;       /* 10 + 1 + 3 + 3 */
    if (app_ef_g() != 7) return 2;
    if (app_ef_K != 3) return 3;
    if (twice(2) != 4 || one() != 1) return 4;
    return 0;
}
