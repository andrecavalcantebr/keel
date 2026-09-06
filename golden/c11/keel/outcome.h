/* keel/outcome.h — perfil C11. `constexpr` vira macro de nome já manglado, mais
   o objeto de conferência (keel-c-backend.md §9.2). O nome de módulo já é único
   no arquivo, então não há reescrita de escopo de bloco aqui. */
#ifndef KEEL_OUTCOME_H
#define KEEL_OUTCOME_H
#include "keel/prelude.h"
#define keel_outcome_OK   ((i32)0)
#define keel_outcome_NONE ((i32)(-2147483647 - 1))
static const i32 keel_outcome_OK__chk   = 0;
static const i32 keel_outcome_NONE__chk = (-2147483647 - 1);
#endif
