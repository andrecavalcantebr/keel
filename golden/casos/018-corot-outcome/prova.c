#include "keel/prelude.h"
#include "keel/corot.impl.h"
#include "keel/keel_outcome_i32.impl.h"
#include "keel/keel_outcome_void.impl.h"
#include "keel/outcome.impl.h"
#include "resultados/resultados.impl.h"
#include <assert.h>
int main(void) {
    keel_corot r = resultados_estado(-7);
    assert(keel_corot_ok(r));
    assert(!keel_corot_ongoing(r) && !keel_corot_faulted(r));
    r = resultados_estado(0);
    assert(keel_corot_ongoing(r));
    assert(!keel_corot_ok(r) && !keel_corot_faulted(r));
    r = resultados_estado(7);
    assert(keel_corot_faulted(r) && keel_corot_code(r) == 7);
    assert(!keel_corot_ok(r) && !keel_corot_ongoing(r));
    assert(!resultados_falhou(-7) && !resultados_falhou(0) && resultados_falhou(7));
    assert(keel_outcome_void_failed(resultados_final(true, -7)));
    assert(keel_outcome_void_failed(resultados_final(true, 7)));
    assert(keel_outcome_void_ok(resultados_final(false, 0)));
    assert(resultados_reparar(-7) == 5 && resultados_reparar(7) == 5);
    /* O verbo modifica o receptor mesmo quando sua cópia de retorno é ignorada. */
    keel_outcome_i32 resultado = {7, 12};
    keel_outcome_i32_win(&resultado);
    assert(keel_outcome_i32_ok(resultado));
    assert(keel_outcome_i32_value(resultado) == 12);
    keel_outcome_i32 copia = keel_outcome_i32_fail(&resultado, -9);
    assert(keel_outcome_i32_code(resultado) == -9 && copia.code == -9);
    assert(resultado.v == 12 && copia.v == 12);
    keel_outcome_i32_value1(&resultado, 42);
    assert(resultado.code == -9 && resultado.v == 42);
    copia = keel_outcome_i32_none(&resultado);
    assert(resultado.code == keel_outcome_NONE && copia.code == keel_outcome_NONE);
    assert(resultado.v == 42 && copia.v == 42);
    assert(keel_outcome_i32_failed(resultado) && !keel_outcome_i32_ok(resultado));
    assert(keel_outcome_i32_code(resultado) == keel_outcome_NONE);
    assert(resultado.code == keel_outcome_NONE && resultado.v == 42);
    keel_outcome_i32 objetos[2] = {{-1, 0}, {-2, 0}};
    size_t indice = 0;
    i32 valor = 80;
    copia = keel_outcome_i32_win1(&objetos[indice++], valor++);
    assert(indice == 1 && valor == 81);
    assert(objetos[0].code == 0 && objetos[0].v == 80);
    assert(copia.code == 0 && copia.v == 80 && objetos[1].code == -2);
    keel_outcome_void sem_valor = {0};
    keel_outcome_void ausencia = keel_outcome_void_none(&sem_valor);
    assert(keel_outcome_void_failed(sem_valor));
    assert(keel_outcome_void_code(ausencia) == keel_outcome_NONE);
    keel_outcome_void_win(&sem_valor);
    assert(keel_outcome_void_ok(sem_valor));
    assert(keel_outcome_void_failed(ausencia));
    i32 contador = 0;
    r = resultados_avaliar_cleanup(&contador);
    assert(keel_corot_code(r) == 1 && contador == 2);
    return 0;
}
