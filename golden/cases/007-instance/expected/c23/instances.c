/* instances.c — generated from instances.k by cgen, C23 profile. */
#include "instances.h"
#include "coll_stack_i32.h"
#line 13 "coll.k"
bool coll_stack_i32_push(coll_stack_i32 *s, i32 v) {
    if (s->len == s->cap) return false;
    s->ptr[s->len++] = v;
    return true;
}
