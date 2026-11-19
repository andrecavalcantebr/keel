/* keel/keel_bitbuffer_1_bool.type.h — generated from keel/bitbuffer.k by cgen, C11 profile. */
#ifndef KEEL_KEEL_BITBUFFER_1_BOOL_TYPE_H
#define KEEL_KEEL_BITBUFFER_1_BOOL_TYPE_H

#include "keel.type.h"

#line 21 "keel/bitbuffer.k"
static_assert(1 >= 1 && 1 <= 8 * sizeof(bool), "bitbuffer-width-invalid: W must be in 1..bits of T");
static_assert((bool)-1 > 0, "bitbuffer-width-invalid: T must be an unsigned integer or bool");
static_assert(!((bool)2 == 1) || 1 == 1, "bitbuffer-width-invalid: a bool element holds one bit");
#line 26 "keel/bitbuffer.k"
typedef struct keel_bitbuffer_1_bool { size_t cap, len; u8 *words; } keel_bitbuffer_1_bool;

#endif /* KEEL_KEEL_BITBUFFER_1_BOOL_TYPE_H */
