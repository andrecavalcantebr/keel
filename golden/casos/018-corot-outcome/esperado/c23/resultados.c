/* resultados.c — gerado de resultados.k pelo cgen, perfil C23. */
#include "resultados.h"
#include "keel/keel_corot.h"
#include "keel/keel_outcome_i32.h"
#include "keel/keel_outcome_void.h"
#line 1 "resultados.k"




keel_corot resultados_estado(i32 codigo) {
    keel_corot r = {0};
    if (codigo > 0) return keel_corot_fault(&r, codigo);
    if (codigo == 0) return keel_corot_again(&r);
    return keel_corot_win(&r);
}

bool resultados_falhou(i32 codigo) {
    keel_corot r = resultados_estado(codigo);
    return keel_corot_faulted(r);
}

keel_outcome_void resultados_final(bool falha, i32 codigo) {
    keel_outcome_void r = {0};
    if (falha) return keel_outcome_void_fail(&r, codigo);
    return keel_outcome_void_win(&r);
}

i32 resultados_reparar(i32 codigo) {
    keel_outcome_i32 origem = {0};
    keel_outcome_i32_fail(&origem, codigo);
    keel_outcome_i32 r = origem; if (keel_outcome_i32_failed(r)) keel_outcome_i32_win1(&r, 5);
    return keel_outcome_i32_value(r);
}



keel_corot resultados_avaliar_cleanup(i32 *contador) {
    keel_corot r = {0};
    keel_corot keel__rv0;
    keel__rv0 = keel_corot_fault(&r, ++*contador); ++*contador; return keel__rv0;
}
