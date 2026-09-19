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

//floats extras
#if defined(__FLT16_MANT_DIG__)
    typedef _Float16 f16;
#endif

#if defined(__FLT128_MANT_DIG__)
    typedef _Float128 f128;
#endif

// OK e NONE nao mencionam o parametro do modulo keel.outcome, entao sao do
// modulo e nao da instancia (backend 4.4.1), e constante de modulo e L1
// (backend 4.3.1). Aqui moram no prelude, que e o unico modulo que existe
// uma vez so nesta ferramenta.
// enum, e nao constexpr: constante de enum nao tem ligacao e pode viver em
// header incluido por muitas unidades.
enum {
    keel_outcome_OK   = 0,
    keel_outcome_NONE = INT32_MIN
};
