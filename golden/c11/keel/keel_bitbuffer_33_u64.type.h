/* keel/keel_bitbuffer_33_u64.type.h — generated from keel/bitbuffer.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BITBUFFER_33_U64_TYPE_H
#define KEEL_KEEL_BITBUFFER_33_U64_TYPE_H

#include "keel.type.h"

#line 21 "keel/bitbuffer.k"
static_assert(33 >= 1 && 33 <= 8 * sizeof(u64), "bitbuffer-width-invalid: W must be in 1..bits of T");
static_assert((u64)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
static_assert(!((u64)2 == 1) || 33 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
#line 26 "keel/bitbuffer.k"
typedef struct keel_bitbuffer_33_u64 { size_t cap, len; u8 *words; } keel_bitbuffer_33_u64;

#endif /* KEEL_KEEL_BITBUFFER_33_U64_TYPE_H */
