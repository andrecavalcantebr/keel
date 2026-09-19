/* resultados.proto.h — gerado de resultados.k pelo cgen, perfil C11. */
#ifndef RESULTADOS_PROTO_H
#define RESULTADOS_PROTO_H
#include "resultados.type.h"

keel_corot resultados_estado(i32 codigo);
bool resultados_falhou(i32 codigo);
keel_outcome_void resultados_final(bool falha, i32 codigo);
i32 resultados_reparar(i32 codigo);
keel_corot resultados_avaliar_cleanup(i32 *contador);
#endif /* RESULTADOS_PROTO_H */
