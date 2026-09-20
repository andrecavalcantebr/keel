/* coll_stack_i32.h — generated from coll.k by cgen, C11 profile. */
#ifndef COLL_STACK_I32_H
#define COLL_STACK_I32_H
#include "coll_stack_i32.type.h"

#line 10 "coll.k"
static inline size_t coll_stack_i32_length(coll_stack_i32 *s);
#line 13 "coll.k"
bool coll_stack_i32_push(coll_stack_i32 *s, i32 v);

#line 10 "coll.k"
static inline size_t coll_stack_i32_length(coll_stack_i32 *s) { return s->len; }
#endif /* COLL_STACK_I32_H */
