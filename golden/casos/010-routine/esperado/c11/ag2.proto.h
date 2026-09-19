/* gen/ag2/ag2.h — gerado de ag2/ag2.k, perfil C11.
   As tabelas são locais das funções, então o `.h` não menciona `routine`. */
#ifndef AG2_H
#define AG2_H
#include "ag2.type.h"

keel_outcome_u32 ag2_cadeia(ag2_Ag *g);
keel_outcome_u32 ag2_cadeia_quebrada(ag2_Ag *g, i32 *estado_terceiro);
keel_outcome_u32 ag2_juntos(ag2_Ag *g);
keel_outcome_u32 ag2_dois(ag2_Ag *g);
keel_outcome_u32 ag2_impossivel(ag2_Ag *g);
#endif /* AG2_H */
