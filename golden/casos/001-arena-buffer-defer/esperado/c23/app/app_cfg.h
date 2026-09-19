/* app/app_cfg.h — gerado de app/cfg.k pelo cgen, perfil C23. */
#ifndef APP_APP_CFG_H
#define APP_APP_CFG_H
#include "app/app_cfg.type.h"

typedef struct keel_arena keel_arena;
keel_outcome_i32 app_cfg_soma(keel_arena *a, const char *caminho);
keel_outcome_i32 app_cfg_soma_scratch(const char *caminho);
#endif /* APP_APP_CFG_H */
