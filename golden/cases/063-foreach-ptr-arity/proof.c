/* proof.c — harness. `sum3` runs a `foreach` with a pointer binder and an
   index binder over `vec i32`, whose `ptr` has a single declared arity. */

#include "keel.type.h"
#include "fv_use.h"

int main(void) {
    if (fv_use_sum3(1, 2, 3) != 6) return 1;
    if (fv_use_sum3(10, -3, 0) != 7) return 2;
    return 0;
}
