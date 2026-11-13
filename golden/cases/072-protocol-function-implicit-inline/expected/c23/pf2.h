/* pf2.h — generated from pf2.k by cgen, C23 profile. */
#ifndef PF2_H
#define PF2_H

#include "pf2.type.h"

#line 8 "pf2.k"
static inline i32 pf2_first(pf2_box *x);
static inline i32 pf2_limit(pf2_box *x);
#line 13 "pf2.k"
i32 pf2_span_of(pf2_box b);

#line 8 "pf2.k"
static inline i32 pf2_first(pf2_box *x) { return x->lo; }
static inline i32 pf2_limit(pf2_box *x) { return x->hi; }

#endif /* PF2_H */
