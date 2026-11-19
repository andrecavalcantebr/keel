/* keel/keel_bitbuffer_5_u8.type.h — generated from keel/bitbuffer.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BITBUFFER_5_U8_TYPE_H
#define KEEL_KEEL_BITBUFFER_5_U8_TYPE_H

#include "keel.type.h"

#line 21 "keel/bitbuffer.k"
static_assert(5 >= 1 && 5 <= 8 * sizeof(u8), "bitbuffer-width-invalid: W must be in 1..bits of T");
static_assert((u8)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
static_assert(!((u8)2 == 1) || 5 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
#line 26 "keel/bitbuffer.k"
typedef struct keel_bitbuffer_5_u8 { size_t cap, len; u8 *words; } keel_bitbuffer_5_u8;

#endif /* KEEL_KEEL_BITBUFFER_5_U8_TYPE_H */
