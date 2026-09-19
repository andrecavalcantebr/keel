/* gen/resultados/resultados.h — gerado de resultados/resultados.k, perfil C23.
   `corot` é tipo, não instância: um header de módulo, sem sufixo de argumento
   (backend §5.14). */
#ifndef RESULTADOS_H
#define RESULTADOS_H
#include "resultados.type.h"

keel_corot resultados_estado(i32 codigo);
bool resultados_falhou(i32 codigo);
keel_outcome_void resultados_final(bool falha, i32 codigo);
i32 resultados_reparar(i32 codigo);
keel_corot resultados_avaliar_cleanup(i32 *contador);
#endif /* RESULTADOS_H */
