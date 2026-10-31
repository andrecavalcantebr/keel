/* proof.c — harness. The set of a `corot` is `Status`, what `tag` returns:
   the arms are exhaustive, and two failure codes are the same arm. */

#include "keel.type.h"
#include "keel/keel_corot.h"
#include "mc.h"
#include <assert.h>
int main(void) {
    keel_corot win = { -1 }, again = { 0 }, f1 = { 7 }, f2 = { 9 };
    assert(mc_classify(win) == 1);
    assert(mc_classify(again) == 2);
    assert(mc_classify(f1) == 7);              /* the failure code stays whole */
    assert(mc_classify(f2) == 9);

    assert(mc_rank(keel_corot_tag(f1)) == 1);  /* two codes, one arm */
    assert(mc_rank(keel_corot_tag(f2)) == 1);
    assert(mc_rank(keel_corot_Status_SUCCESS) == -1);
    assert(mc_rank(keel_corot_Status_ONGOING) == 0);
    return 0;
}
