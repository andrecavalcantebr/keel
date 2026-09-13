/* prova.c — arnês. O resultado da composição é o código da política mais a
   contagem de sucessos; o que cada participante deixou fica no seu slot. */
#include "ag2/ag2.h"
#include <assert.h>

int main(void) {
    ag2_Ag g = { 2, 1, 0 };
    keel_outcome_u32 r = ag2_cadeia(&g);          /* três etapas, todas vencem */
    assert(keel_outcome_u32_ok(r));
    assert(keel_outcome_u32_value(r) == 3);

    i32 terceiro = -1;
    g = (ag2_Ag){ 1, 0, 0 };
    r = ag2_cadeia_quebrada(&g, &terceiro);        /* a segunda falha          */
    assert(keel_outcome_u32_failed(r));
    assert(keel_outcome_u32_value(r) == 1);        /* uma etapa venceu antes   */
    assert(terceiro == 0);                         /* a terceira ficou ONGOING */

    g = (ag2_Ag){ 0, 3, 0 };
    r = ag2_juntos(&g);                            /* alvo um entre duas       */
    assert(keel_outcome_u32_ok(r));
    assert(keel_outcome_u32_value(r) == 1);        /* só a primeira venceu     */
    assert(g.b == 2);                              /* a segunda foi chamada    */

    g = (ag2_Ag){ 0, 0, 0 };
    r = ag2_dois(&g);                              /* uma falha, alvo dois     */
    assert(keel_outcome_u32_ok(r));
    assert(keel_outcome_u32_value(r) == 2);

    g = (ag2_Ag){ 0, 0, 5 };
    r = ag2_impossivel(&g);                        /* duas falhas, alvo dois   */
    assert(keel_outcome_u32_failed(r));
    assert(keel_outcome_u32_value(r) == 0);
    return 0;
}
