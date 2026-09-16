/* gen/app/cfg.h — gerado de app/cfg.k, perfil C11 */
#ifndef APP_APP_CFG_H
#define APP_APP_CFG_H
#include "app/app_cfg.type.h"

/* `keel_arena *` e ponteiro: basta o nome (backend 4.3.1) */
typedef struct keel_arena keel_arena;

#define app_cfg_MAX ((size_t)256)
static const size_t app_cfg_MAX__chk = 256;
keel_outcome_i32 app_cfg_soma(keel_arena *a, const char *caminho);
keel_outcome_i32 app_cfg_soma_scratch(const char *caminho);
#endif /* APP_APP_CFG_H */
