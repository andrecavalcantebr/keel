/* app/app_pool.c — gerado de app/pool.k, perfil C11.
   O construtor não recebe alinhamento: quem alinha é a alocação (backend §5.4). */

#include "keel.type.h"
#include "app/app_pool.h"
#include "keel/keel_arena.h"
#line 6 "app/pool.k"
_Alignas(64) static u8 app_pool_memo[65536];

bool app_pool_inicia(keel_arena *a) {
    return keel_arena_from_array(a, app_pool_memo, sizeof app_pool_memo);
}
