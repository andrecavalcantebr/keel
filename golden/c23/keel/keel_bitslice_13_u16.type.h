/* keel/keel_bitslice_13_u16.type.h — generated from keel/bitslice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_BITSLICE_13_U16_TYPE_H
#define KEEL_KEEL_BITSLICE_13_U16_TYPE_H

#include "keel.type.h"

#line 19 "keel/bitslice.k"
static_assert(13 >= 1 && 13 <= 8 * sizeof(u16), "bitbuffer-width-invalid: W must be in 1..bits of T");
static_assert((u16)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
static_assert(!((u16)2 == 1) || 13 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
#line 25 "keel/bitslice.k"
typedef struct keel_bitslice_13_u16 { size_t len; size_t bit; u8 *words; } keel_bitslice_13_u16;

#endif /* KEEL_KEEL_BITSLICE_13_U16_TYPE_H */
