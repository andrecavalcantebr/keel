/* proof.c — harness. The three forms of mf.k that the first `match` lowering refused:
   the operand is a field of a struct, the arms keep their statement on the label's line,
   and a `break` of the arm sits next to a loop that has its own. */

#include "keel.type.h"
#include "app/app_mf.h"
#include "keel/keel_tagged_app_mf_Cycle_void.type.h"

int main(void) {
    app_mf_Holder h = { { app_mf_Cycle_A } };
    if (app_mf_member(&h) != 1) return 1;
    h.state.tag = app_mf_Cycle_B;
    if (app_mf_member(&h) != 2) return 2;
    if (app_mf_inline_arms() != 2) return 3;
    if (app_mf_with_loop(5) != 4) return 4;          /* the loop adds 0 + 1 + 2 and breaks at 3 */
    return 0;
}
