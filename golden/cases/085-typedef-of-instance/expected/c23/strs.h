/* strs.h — generated from strs.k by cgen, C23 profile. */
#ifndef STRS_H
#define STRS_H

#include "strs.type.h"

#line 8 "strs.k"
static inline size_t strs_size(strs_text *t);

#include "keel/keel_buffer_u8.h"

#line 8 "strs.k"
static inline size_t strs_size(strs_text *t) { return keel_buffer_u8_length(&*t); }

#endif /* STRS_H */
