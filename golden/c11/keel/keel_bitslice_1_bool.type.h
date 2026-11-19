/* keel/keel_bitslice_1_bool.type.h — generated from keel/bitslice.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BITSLICE_1_BOOL_TYPE_H
#define KEEL_KEEL_BITSLICE_1_BOOL_TYPE_H

#include "keel.type.h"

#line 19 "keel/bitslice.k"
static_assert(1 >= 1 && 1 <= 8 * sizeof(bool), "bitbuffer-width-invalid: W must be in 1..bits of T");
static_assert((bool)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
static_assert(!((bool)2 == 1) || 1 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
#line 25 "keel/bitslice.k"
typedef struct keel_bitslice_1_bool { size_t len; size_t bit; u8 *words; } keel_bitslice_1_bool;

#endif /* KEEL_KEEL_BITSLICE_1_BOOL_TYPE_H */
