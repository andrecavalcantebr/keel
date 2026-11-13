/* pf2_span_pf2_box.h — generated from pf2.k by cgen, C23 profile. */
#ifndef PF2_SPAN_PF2_BOX_H
#define PF2_SPAN_PF2_BOX_H

#include "pf2_span_pf2_box.type.h"

#line 11 "pf2.k"
static inline i32 pf2_span_pf2_box(pf2_box items);

#include "pf2.h"

#line 11 "pf2.k"
static inline i32 pf2_span_pf2_box(pf2_box items) { return pf2_limit(&items) - pf2_first(&items); }

#endif /* PF2_SPAN_PF2_BOX_H */
