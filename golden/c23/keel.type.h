/* keel.type.h — generated from keel.k by cgen, C23 profile. */
#ifndef KEEL_TYPE_H
#define KEEL_TYPE_H
#include <stdint.h>
#include <stddef.h>
#include <float.h>

#line 19 "keel.k"
typedef int8_t  i8;   typedef uint8_t  u8;
#line 20 "keel.k"
typedef int16_t i16;  typedef uint16_t u16;
#line 21 "keel.k"
typedef int32_t i32;  typedef uint32_t u32;
#line 22 "keel.k"
typedef int64_t i64;  typedef uint64_t u64;
#line 23 "keel.k"
typedef float   f32;  typedef double   f64;

static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128
              && sizeof(f32) == 4, "keel: f32 requires IEEE 754 binary32 on this target");
static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024
              && sizeof(f64) == 8, "keel: f64 requires IEEE 754 binary64 on this target");
#endif /* KEEL_TYPE_H */
