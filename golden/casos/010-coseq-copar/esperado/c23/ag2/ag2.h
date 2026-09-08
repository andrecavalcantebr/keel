/* gen/ag2/ag2.h — gerado de ag2/ag2.k, perfil C23. */
#ifndef AG2_AG2_H
#define AG2_AG2_H
#include "keel/prelude.h"
#include "keel/keel_corot_i32.h"
#include "keel/keel_outcome_i32.h"

typedef struct { int a, b, c; } ag2_Ag;

keel_outcome_i32 ag2_cadeia(ag2_Ag *g);
keel_outcome_i32 ag2_cadeia_quebrada(ag2_Ag *g);
keel_outcome_i32 ag2_juntos(ag2_Ag *g);
#endif
