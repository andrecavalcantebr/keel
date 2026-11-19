/* keel/keel_bitslice_4_u8.type.h — generated from keel/bitslice.k by cgen, C23 profile. */
#ifndef KEEL_KEEL_BITSLICE_4_U8_TYPE_H
#define KEEL_KEEL_BITSLICE_4_U8_TYPE_H

#include "keel.type.h"

#line 19 "keel/bitslice.k"
static_assert(4 >= 1 && 4 <= 8 * sizeof(u8), "bitbuffer-width-invalid: W must be in 1..bits of T");
static_assert((u8)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
static_assert(!((u8)2 == 1) || 4 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
#line 25 "keel/bitslice.k"
typedef struct keel_bitslice_4_u8 { size_t len; size_t bit; u8 *words; } keel_bitslice_4_u8;

#endif /* KEEL_KEEL_BITSLICE_4_U8_TYPE_H */
