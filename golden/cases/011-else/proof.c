/* proof.c — harness for both forms of the `else` clause. */

#include "keel.type.h"
#include "app/app_el.h"
#include <stdio.h>
int main(void) {
    i32 base[4] = {1,2,3,4};
    if (app_el_twice(5)  != 10) return 1;
    if (app_el_twice(-1) != -1) return 2;      /* fail(7) → exit */
    if (app_el_twice(0)  != -1) return 3;      /* NONE is a failure too */
    if (app_el_twice_log(3) != 6) return 4;
    if (app_el_size(3, base) != 3) return 5;
    if (app_el_size(0, base) != 0) return 6; /* the default repaired the symbol */
    puts("ok");
    return 0;
}
