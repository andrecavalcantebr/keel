/* fv_vec_i32.h — generated from fv.k by cgen, C23 profile. */
#ifndef FV_VEC_I32_H
#define FV_VEC_I32_H

#include "fv_vec_i32.type.h"

#line 8 "fv.k"
static inline size_t fv_vec_i32_length(fv_vec_i32 *v);
static inline i32 *fv_vec_i32_ptr(fv_vec_i32 *v, size_t i);

#line 8 "fv.k"
static inline size_t fv_vec_i32_length(fv_vec_i32 *v) { return v->len; }
static inline i32 *fv_vec_i32_ptr(fv_vec_i32 *v, size_t i) { return &v->data[i]; }

#endif /* FV_VEC_I32_H */
