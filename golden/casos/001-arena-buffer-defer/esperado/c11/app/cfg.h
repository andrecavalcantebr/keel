/* gen/app/cfg.h — gerado de app/cfg.k, perfil C11.
   Só o que é `pub`: o tipo, a constante e os protótipos (backend §4.1). */
#ifndef APP_CFG_H
#define APP_CFG_H
#include "keel/prelude.h"
#include "keel/arena.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_outcome_i32.h"

#define app_cfg_MAX ((size_t)256)
static const size_t app_cfg_MAX__chk = 256;

keel_outcome_i32 app_cfg_soma(arena *a, const char *caminho);
keel_outcome_i32 app_cfg_soma_scratch(const char *caminho);
#endif
