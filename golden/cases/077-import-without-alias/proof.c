/* proof.c — harness. `import app.qty;` without `as`: the qualifier at the use site is the
   module's own name in full, `app.qty.SEVEN` and `app.qty.triple(2)` (spec §4.1 regra 5). */

#include "keel.type.h"
#include "app/app_qty_use.h"

int main(void) {
    return app_qty_use_total() == 7 + 6 ? 0 : 1;
}
