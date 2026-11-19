/* proof.c — harness: the call of a function of the module's fixed part, from
   outside and from an instance's own verb, and the instance that carries its
   `static_assert`. */
#include "keel.type.h"
#include "app/app_gfp_use.h"
#include <stdio.h>
int main(void) {
    if (app_gfp_use_outside() != 42) return 1;
    if (app_gfp_use_inside() != 14) return 2;
    puts("ok");
    return 0;
}
