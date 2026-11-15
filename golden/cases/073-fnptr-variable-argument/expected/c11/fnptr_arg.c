/* fnptr_arg.c — generated from fnptr_arg.k by cgen, C11 profile. */
#include "fnptr_arg.h"
#line 1 "fnptr_arg.k"


static u64 fnptr_arg_hash(const i32 *k) { return (u64)*k; }
static u64 fnptr_arg_apply(u64 (*f)(const i32 *), const i32 *x) { return f(x); }
static u64 fnptr_arg_forward(u64 (*h)(const i32 *), const i32 *x) { return fnptr_arg_apply(h, x); }

u64 fnptr_arg_run(void) { i32 v = 3; return fnptr_arg_forward(fnptr_arg_hash, &v); }
