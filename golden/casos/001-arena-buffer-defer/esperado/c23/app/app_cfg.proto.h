/* gen/app/cfg.h — gerado de app/cfg.k, perfil C23.
   Só o que é `pub`: o tipo, a constante e os protótipos (backend §4.1). */
#ifndef APP_APP_CFG_H
#define APP_APP_CFG_H
#include "app/app_cfg.type.h"

typedef struct keel_arena keel_arena;
constexpr size_t app_cfg_MAX = 256;
keel_outcome_i32 app_cfg_soma(keel_arena *a, const char *caminho);
keel_outcome_i32 app_cfg_soma_scratch(const char *caminho);
#endif /* APP_APP_CFG_H */
