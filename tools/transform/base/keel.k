/*
 * module keel
 *
 * DESC: keel prelude for parser keel
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3
 */

%type
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//signed ints
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

//unsigned ints
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

//floats
typedef float  f32;
typedef double f64;

//extra floats
#if defined(__FLT16_MANT_DIG__)
    typedef _Float16 f16;
#endif

#if defined(__FLT128_MANT_DIG__)
    typedef _Float128 f128;
#endif

// OK and NONE do not mention keel.outcome's parameter, so they belong to the
// module, not the instance (backend 4.4.1), and a module constant is L1
// (backend 4.3.1). They live in the prelude here, the only module this tool
// emits just once.
// enum, not constexpr: an enum constant has no linkage and can live in a
// header included by many translation units.
enum {
    keel_outcome_OK   = 0,
    keel_outcome_NONE = INT32_MIN
};
