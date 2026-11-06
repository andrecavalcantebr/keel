/* pf.h — generated from pf.k by cgen, C11 profile. */
#ifndef PF_H
#define PF_H

#include "pf.type.h"

#line 9 "pf.k"
static inline i32 pf_first(pf_box *x);
static inline i32 pf_limit(pf_box *x);
#line 14 "pf.k"
i32 pf_span_of(pf_box b);

#line 9 "pf.k"
static inline i32 pf_first(pf_box *x) { return x->lo; }
static inline i32 pf_limit(pf_box *x) { return x->hi; }

#endif /* PF_H */
