/* prova.c — arnês das duas políticas de avanço. */
#include "ag2/ag2.h"
#include <stdio.h>

int main(void) {
    /* coseq: três etapas, cada uma cedendo duas vezes antes de terminar */
    ag2_Ag g = { 2, 2, 2 };
    keel_outcome_i32 r = ag2_cadeia(&g);
    if (keel_outcome_i32_failed(r)) return 1;
    if (keel_outcome_i32_value(r) != 30) return 2;   /* valor da ÚLTIMA etapa */

    /* falha na etapa 2 → código 2, o ordinal base um */
    g = (ag2_Ag){ 0, 0, 0 };
    r = ag2_cadeia_quebrada(&g);
    if (!keel_outcome_i32_failed(r)) return 3;
    if (keel_outcome_i32_code(r) != 2) return 4;

    /* copar ANY: `ola` termina primeiro, e o bloco guardado impede que
       `autentica` seja chamado de novo — foi chamado exatamente uma vez */
    g = (ag2_Ag){ 0, 5, 0 };
    r = ag2_juntos(&g);
    if (keel_outcome_i32_failed(r)) return 5;
    if (g.b != 4) return 6;
    puts("ok");
    return 0;
}
