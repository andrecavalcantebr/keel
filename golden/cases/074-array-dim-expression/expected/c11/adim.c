/* adim.c — generated from adim.k by cgen, C11 profile. */
#include "adim.h"
#include "keel/keel_arena.h"
#line 1 "adim.k"





static u8 adim_memo[1<<10];

bool adim_start(keel_arena *a) {
    return keel_arena_from_array(a, adim_memo, 1<<10);
}
