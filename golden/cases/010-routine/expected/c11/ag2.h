/* ag2.h — generated from ag2.k by cgen, C11 profile. */
#ifndef AG2_H
#define AG2_H
#include "ag2.type.h"
#include "keel/keel_outcome_u32.type.h"

#line 16 "ag2.k"
keel_outcome_u32 ag2_chain(ag2_Ag *g);
#line 27 "ag2.k"
keel_outcome_u32 ag2_broken_chain(ag2_Ag *g, i32 *third_state);
#line 40 "ag2.k"
keel_outcome_u32 ag2_together(ag2_Ag *g);
#line 49 "ag2.k"
keel_outcome_u32 ag2_two(ag2_Ag *g);
#line 59 "ag2.k"
keel_outcome_u32 ag2_impossible(ag2_Ag *g);
#endif /* AG2_H */
