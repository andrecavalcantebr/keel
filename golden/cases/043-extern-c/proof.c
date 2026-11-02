/* proof.c — harness. The plain `extern_c` is in the .h, where the next prototype can use it;
   the `[type_h]` one is in the .type.h; the `priv` one stays in the .c. */

#include "keel.type.h"
#include "app/app_xc.h"

int main(void) {
    Raw r = { 5 };
    if (app_xc_before(&r) != 5) return 1;
    if (app_xc_after(&r) != 10) return 2;
    if (twice(4) != 8) return 3;               /* visible to any includer of the .h */
    if (app_xc_hidden() != 7) return 4;
    return 0;
}
