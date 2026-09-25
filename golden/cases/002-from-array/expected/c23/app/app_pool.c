/* app/app_pool.c — generated from app/pool.k by cgen, C23 profile. */
#include "app/app_pool.h"
#include "keel/keel_arena.h"
#line 1 "app/pool.k"






static alignas(64) u8 app_pool_memo[65536];

bool app_pool_start(keel_arena *a) {
    return keel_arena_from_array(a, app_pool_memo, sizeof app_pool_memo);
}
