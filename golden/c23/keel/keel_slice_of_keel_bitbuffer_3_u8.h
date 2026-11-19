/* keel/keel_slice_of_keel_bitbuffer_3_u8.h — generated from keel/slice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_SLICE_OF_KEEL_BITBUFFER_3_U8_H
#define KEEL_KEEL_SLICE_OF_KEEL_BITBUFFER_3_U8_H

#include "keel/keel_slice_of_keel_bitbuffer_3_u8.type.h"
#include "keel/keel_bitbuffer_3_u8.type.h"
#include "keel/keel_range.type.h"
#include "keel/keel_bitslice_3_u8.type.h"

#line 54 "keel/slice.k"
static inline keel_bitslice_3_u8 keel_slice_of_keel_bitbuffer_3_u8(keel_bitbuffer_3_u8 *x);
static inline keel_bitslice_3_u8 keel_slice_of1_keel_bitbuffer_3_u8(keel_bitbuffer_3_u8 *x, keel_range r);
static inline keel_bitslice_3_u8 keel_slice_of2_keel_bitbuffer_3_u8(keel_bitbuffer_3_u8 *x, size_t a, size_t b);

#include "keel/keel_bitbuffer_3_u8.h"

#line 54 "keel/slice.k"
static inline keel_bitslice_3_u8 keel_slice_of_keel_bitbuffer_3_u8(keel_bitbuffer_3_u8 *x) { return keel_bitbuffer_3_u8_as_slice2(x, 0, keel_bitbuffer_3_u8_length(x)); }
static inline keel_bitslice_3_u8 keel_slice_of1_keel_bitbuffer_3_u8(keel_bitbuffer_3_u8 *x, keel_range r) { return keel_bitbuffer_3_u8_as_slice2(x, r.first, r.limit); }
static inline keel_bitslice_3_u8 keel_slice_of2_keel_bitbuffer_3_u8(keel_bitbuffer_3_u8 *x, size_t a, size_t b) { return keel_bitbuffer_3_u8_as_slice2(x, a, b); }

#endif /* KEEL_KEEL_SLICE_OF_KEEL_BITBUFFER_3_U8_H */
