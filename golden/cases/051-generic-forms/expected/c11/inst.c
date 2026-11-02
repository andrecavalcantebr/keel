/* inst.c — generated from inst.k by cgen, C11 profile. */
#include "inst.h"
#include "gx_stack_i32.h"
#include "gx_cell_i32.h"
#line 10 "gx.k"
static i32 gx_cell_i32_peek(gx_cell_i32 *c) { return c->v; }
#line 12 "gx.k"
i32 gx_cell_i32_get(gx_cell_i32 *c) { return gx_cell_i32_peek(c); }
