/* gen/app/cfg.h — gerado de app/cfg.k, perfil C23.
   Só o que é `pub`: o tipo, a constante e os protótipos (backend §4.1). */
#ifndef APP_CFG_H
#define APP_CFG_H
#include "app/cfg.type.h"

typedef struct arena arena;
constexpr size_t app_cfg_MAX = 256;
keel_outcome_i32 app_cfg_soma(arena *a, const char *caminho);
keel_outcome_i32 app_cfg_soma_scratch(const char *caminho);
#endif /* APP_CFG_H */
