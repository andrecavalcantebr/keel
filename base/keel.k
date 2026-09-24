/*
 * module keel
 *
 * DESC: keel — the language prelude: primitive types (spec §4.2, §5.1; backend §4.2)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel;

import_c <stdint.h>;
import_c <stddef.h>;
import_c <float.h>;

// fixed widths, signed and unsigned (spec §4.2). The core grammar already
// recognizes the name in type position; this module only supplies the C
// behind it — the one place in the backend where <stdint.h> appears.
pub typedef int8_t  i8;   pub typedef uint8_t  u8;
pub typedef int16_t i16;  pub typedef uint16_t u16;
pub typedef int32_t i32;  pub typedef uint32_t u32;
pub typedef int64_t i64;  pub typedef uint64_t u64;
pub typedef float   f32;  pub typedef double   f64;

// proves the IEEE 754 format, not just the width (backend §4.2): sizeof == 4
// alone cannot tell binary32 from a 32-bit float that is not IEEE.
static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128
              && sizeof(f32) == 4, "keel: f32 requires IEEE 754 binary32 on this target");
static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024
              && sizeof(f64) == 8, "keel: f64 requires IEEE 754 binary64 on this target");

// bool, char, size_t, ptrdiff_t and uintptr_t keep their C contracts (spec
// §4.2): they are not remapped here. stdbool.h/assert.h under C11, and the
// spelling of the assert per profile, are the backend's choice at generation
// time — not this source's.

// debug checks (backend §5.17): the switch is a C macro, not a difference in
// generation. `cgen --checks=off` passes -DKEEL_CHECKS=0 to the C compiler, and
// the generated C is the same in both modes. Switched off, the condition is not
// evaluated — `sizeof` only keeps its operands used.
pub extern_c [type_h] {
#ifndef KEEL_CHECKS
#define KEEL_CHECKS 1
#endif
#if KEEL_CHECKS
#include <stdio.h>
#include <stdlib.h>
#define KEEL_CHECK(cond, id) \
    ((cond) ? (void)0 : (fprintf(stderr, "%s:%d: keel: %s\n", __FILE__, __LINE__, (id)), abort()))
#else
#define KEEL_CHECK(cond, id) ((void)sizeof((cond) ? 1 : 0))
#endif
static inline size_t keel_index(size_t i, size_t d) {
    KEEL_CHECK(i < d, "array-index-out-of-bounds");
    return i;
}
}

// The core operations over `array` — keel.length, capacity, dim, get, set, ptr
// and at (spec §4.2) — are qualified by this module but not declared here: they
// are generic over the element and read the extent from the declaration, which
// no keel declaration can express. Their lowering is the backend's (backend
// §5.2); get, set and ptr(v, i) go through keel_index above.
