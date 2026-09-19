/* ag2.h — gerado de ag2.k pelo cgen, perfil C23. */
#ifndef AG2_H
#define AG2_H
#include "ag2.type.h"
#include "keel/keel_outcome_u32.type.h"

#line 16 "ag2.k"
keel_outcome_u32 ag2_cadeia(ag2_Ag *g);
#line 27 "ag2.k"
keel_outcome_u32 ag2_cadeia_quebrada(ag2_Ag *g, i32 *estado_terceiro);
#line 40 "ag2.k"
keel_outcome_u32 ag2_juntos(ag2_Ag *g);
#line 49 "ag2.k"
keel_outcome_u32 ag2_dois(ag2_Ag *g);
#line 59 "ag2.k"
keel_outcome_u32 ag2_impossivel(ag2_Ag *g);
#endif
