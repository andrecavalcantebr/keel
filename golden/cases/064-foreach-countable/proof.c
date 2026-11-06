/* proof.c — harness. `sum_param` and `sum_local` run the one-binder `foreach`
   over `window`, which declares only `first` and `limit` (`Countable`, spec
   §5.1) — no `get` nor `ptr`. */

#include "keel.type.h"
#include "win.h"

int main(void) {
    win_window w = { 2, 5 };
    if (win_sum_param(&w) != 9) return 1;   /* [2, 5): 2+3+4 = 9 */
    if (win_sum_local() != 9) return 2;
    return 0;
}
