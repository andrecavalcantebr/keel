/* gen/app/pool.c — gerado de app/pool.k, perfil C23.
   O construtor não recebe alinhamento: quem alinha é a alocação (backend §5.4). */
#include "app/pool.h"

#line 6 "app/pool.k"
alignas(64) static u8 app_pool_memo[65536];

bool app_pool_inicia(keel_arena *a) {
    return keel_arena_from_array(a, app_pool_memo, sizeof app_pool_memo);
}
