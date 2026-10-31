/* mc.h — generated from mc.k by cgen, C11 profile. */
#ifndef MC_H
#define MC_H

#include "mc.type.h"
#include "keel/keel_corot.type.h"

typedef struct keel_corot keel_corot;
#line 7 "mc.k"
i32 mc_classify(keel_corot r);
#line 23 "mc.k"
i32 mc_rank(keel_corot_Status s);
#line 40 "mc.k"
i32 mc_classify_ptr(keel_corot *r);
#line 55 "mc.k"
i32 mc_rank_ptr(keel_corot_Status *s);

#endif /* MC_H */
