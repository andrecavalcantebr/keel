/* proof.c — harness. `manual` owns the loop and the dispatch; `set_of` composes two
   participants with `routine.par`, whose target is reached at the end of a cycle. */

#include "keel.type.h"
#include "keel/keel_corot.h"
#include "keel/keel_outcome_u32.h"
#include "coop.h"
#include <assert.h>
int main(void) {
    assert(coop_manual() == 2);

    i16 a = 0, b = 0;
    keel_outcome_u32 r = coop_set_of(&a, &b);
    assert(keel_outcome_u32_ok(r));
    assert(keel_outcome_u32_value(r) == 2);    /* both finished in the cycle that reached the target */
    assert(a == 4 && b == 2);                  /* the traversal noted both slots, through `a` */
    return 0;
}
