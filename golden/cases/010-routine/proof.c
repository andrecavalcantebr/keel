/* proof.c — harness. The composition's result is the policy's code plus the
   count of successes; what each participant left stays in its slot. */

#include "keel.type.h"
#include "ag2.h"
#include "keel/keel_outcome_u32.h"
#include <assert.h>
int main(void) {
    ag2_Ag g = { 2, 1, 0 };
    keel_outcome_u32 r = ag2_chain(&g);          /* three stages, all of them win */
    assert(keel_outcome_u32_ok(r));
    assert(keel_outcome_u32_value(r) == 3);

    i32 third = -1;
    g = (ag2_Ag){ 1, 0, 0 };
    r = ag2_broken_chain(&g, &third);        /* a segunda falha          */
    assert(keel_outcome_u32_failed(r));
    assert(keel_outcome_u32_value(r) == 1);        /* uma etapa venceu antes   */
    assert(third == 0);                         /* a terceira ficou ONGOING */

    g = (ag2_Ag){ 0, 3, 0 };
    r = ag2_together(&g);                            /* alvo um entre duas       */
    assert(keel_outcome_u32_ok(r));
    assert(keel_outcome_u32_value(r) == 1);        /* only the first one won   */
    assert(g.b == 2);                              /* a segunda foi chamada    */

    g = (ag2_Ag){ 0, 0, 0 };
    r = ag2_two(&g);                              /* uma falha, alvo dois     */
    assert(keel_outcome_u32_ok(r));
    assert(keel_outcome_u32_value(r) == 2);

    g = (ag2_Ag){ 0, 0, 5 };
    r = ag2_impossible(&g);                        /* duas falhas, alvo dois   */
    assert(keel_outcome_u32_failed(r));
    assert(keel_outcome_u32_value(r) == 0);
    return 0;
}
