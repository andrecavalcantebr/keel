/* twos_out_i32.h — generated from twos.k by cgen, C11 profile. */
#ifndef TWOS_OUT_I32_H
#define TWOS_OUT_I32_H

#include "twos_out_i32.type.h"

#line 7 "twos.k"
static inline void twos_out_i32_fill(twos_out_i32 *o, i32 v);

#line 7 "twos.k"
static inline void twos_out_i32_fill(twos_out_i32 *o, i32 v) { for (size_t i = 0; i < o->n; i++) o->data[i] = v; }

#endif /* TWOS_OUT_I32_H */
