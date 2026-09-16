/* gen/resultados/resultados.c — gerado de resultados/resultados.k, perfil C11.
   Os produtores de `corot` são verbos: ajustam o receptor e devolvem a cópia.
   Quem sai da função é o `return` escrito no fonte (backend §5.14). */
#include "keel/prelude.h"
#include "keel/corot.impl.h"
#include "keel/keel_outcome_i32.impl.h"
#include "keel/keel_outcome_void.impl.h"
#include "resultados/resultados.impl.h"
#line 5 "resultados/resultados.k"
keel_corot resultados_estado(i32 codigo) {
    keel_corot r = {0};
    if (codigo > 0) return keel_corot_fault(&r, codigo);
    if (codigo == 0) return keel_corot_again(&r);
    return keel_corot_win(&r);
}

#line 12 "resultados/resultados.k"
bool resultados_falhou(i32 codigo) {
    keel_corot r = resultados_estado(codigo);
    return keel_corot_faulted(r);
}

#line 17 "resultados/resultados.k"
keel_outcome_void resultados_final(bool falha, i32 codigo) {
    keel_outcome_void r = {0};
    if (falha) return keel_outcome_void_fail(&r, codigo);
    return keel_outcome_void_win(&r);
}

#line 23 "resultados/resultados.k"
i32 resultados_reparar(i32 codigo) {
    keel_outcome_i32 origem = {0};
    keel_outcome_i32_fail(&origem, codigo);
    keel_outcome_i32 r = origem; if (keel_outcome_i32_failed(r)) keel_outcome_i32_win1(&r, 5);
    return keel_outcome_i32_value(r);
}

#line 32 "resultados/resultados.k"
keel_corot resultados_avaliar_cleanup(i32 *contador) {
    keel_corot r = {0};
    keel_corot keel__rv0 = keel_corot_fault(&r, ++*contador);
    ++*contador;
    return keel__rv0;
}
