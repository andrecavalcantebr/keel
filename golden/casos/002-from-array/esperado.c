/* gen/app/pool.c — o que keel-c-backend.md §5.4 especifica hoje.
   NAO COMPILA: `alignof` sobre objeto nao e C padrao. Ver PROBLEMA. */
#include "keel/prelude.h"
#include "keel/arena.h"

alignas(64) u8 app_pool_memo[65536];

bool app_pool_inicia(keel_arena *a) {
    return keel_arena_from_array(a, app_pool_memo, sizeof app_pool_memo,
                                 alignof(app_pool_memo));
}
