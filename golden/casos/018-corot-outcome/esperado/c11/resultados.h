/* resultados.h — gerado de resultados.k pelo cgen, perfil C11. */
#ifndef RESULTADOS_H
#define RESULTADOS_H
#include "resultados.type.h"
#include "keel/keel_corot.type.h"
#include "keel/keel_outcome_void.type.h"

#line 5 "resultados.k"
keel_corot resultados_estado(i32 codigo);
#line 12 "resultados.k"
bool resultados_falhou(i32 codigo);
#line 17 "resultados.k"
keel_outcome_void resultados_final(bool falha, i32 codigo);
#line 23 "resultados.k"
i32 resultados_reparar(i32 codigo);
#line 32 "resultados.k"
keel_corot resultados_avaliar_cleanup(i32 *contador);
#endif /* RESULTADOS_H */
