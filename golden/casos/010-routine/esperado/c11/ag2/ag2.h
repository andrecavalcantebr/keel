/* gen/ag2/ag2.h — gerado de ag2/ag2.k, perfil C11.
   As tabelas são locais das funções, então o `.h` não menciona `routine`. */
#ifndef AG2_AG2_H
#define AG2_AG2_H
#include "keel/prelude.h"
#include "keel/keel_outcome_u32.h"

typedef struct { i32 a, b, c; } ag2_Ag;

keel_outcome_u32 ag2_cadeia(ag2_Ag *g);
keel_outcome_u32 ag2_cadeia_quebrada(ag2_Ag *g, i32 *estado_terceiro);
keel_outcome_u32 ag2_juntos(ag2_Ag *g);
keel_outcome_u32 ag2_dois(ag2_Ag *g);
keel_outcome_u32 ag2_impossivel(ag2_Ag *g);
#endif
