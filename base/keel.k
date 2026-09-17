/*
 * module keel
 *
 * DESC: keel — prelúdio da linguagem: tipos primitivos (spec §4.2, §5.1; backend §4.2)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — ver /LICENSE.md
 */

module keel;

import_c <stdint.h>;
import_c <stddef.h>;
import_c <float.h>;

// larguras fixas, com e sem sinal (spec §4.2). O nome já é reconhecido pela
// gramática do núcleo em posição de tipo; este módulo só dá o C que o
// sustenta — é o único lugar do backend em que <stdint.h> aparece.
pub typedef int8_t  i8;   pub typedef uint8_t  u8;
pub typedef int16_t i16;  pub typedef uint16_t u16;
pub typedef int32_t i32;  pub typedef uint32_t u32;
pub typedef int64_t i64;  pub typedef uint64_t u64;
pub typedef float   f32;  pub typedef double   f64;

// prova o formato IEEE 754, não só a largura (backend §4.2): sizeof == 4
// sozinho não distingue binary32 de um float de 32 bits que não é IEEE.
static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128
              && sizeof(f32) == 4, "keel: f32 exige IEEE 754 binary32 neste alvo");
static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024
              && sizeof(f64) == 8, "keel: f64 exige IEEE 754 binary64 neste alvo");

// bool, char, size_t, ptrdiff_t e uintptr_t conservam seus contratos C (spec
// §4.2): não são remapeados aqui. stdbool.h/assert.h sob C11, e a grafia do
// assert por perfil, são escolha do backend na geração — não deste fonte.
