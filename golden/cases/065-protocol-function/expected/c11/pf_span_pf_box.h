/* pf_span_pf_box.h — generated from pf.k by cgen, C11 profile. */
#ifndef PF_SPAN_PF_BOX_H
#define PF_SPAN_PF_BOX_H

#include "pf_span_pf_box.type.h"

#line 12 "pf.k"
static inline i32 pf_span_pf_box(pf_box items);

#include "pf.h"

#line 12 "pf.k"
static inline i32 pf_span_pf_box(pf_box items) { return pf_limit(&items) - pf_first(&items); }

#endif /* PF_SPAN_PF_BOX_H */
